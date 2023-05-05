#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "kthread.h"

extern struct proc proc[NPROC];

void kthreadinit(struct proc *p)
{
  // initlock(&p->tid_lock, "nexttid");
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->t_lock, "threadlock");
    // acquire(&kt->lock);
    kt->state = T_UNUSED;
    kt->parent = p;

    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}

struct kthread *mykthread()
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *mythread = c->kthread;
  pop_off();
  return mythread;
}

struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}

// TODO: delte this after you are done with task 2.2
// void allocproc_help_function(struct proc *p)
// {
//   p->kthread->trapframe = get_kthread_trapframe(p, p->kthread);

//   p->context.sp = p->kthread->kstack + PGSIZE;
// }
int alloctid(struct proc *p)
{
  int tid;
  acquire(&p->tid_lock);
  tid = p->counter;
  p->counter++;
  release(&p->tid_lock);
  return tid;
}

struct kthread *allockthread(struct proc *p)
{
  struct kthread *t;
  // acquire(&p->lock); - check if the lock is allready aquired in allocproc
  for (t = p->kthread; t < &p->kthread[NKT]; t++)
  {
    acquire(&t->t_lock);
    if (t->state == T_UNUSED)
    {
      t->tid = alloctid(p);
      t->state = T_USED;
      t->trapframe = get_kthread_trapframe(p, t);
      memset(&t->context, 0, sizeof(t->context));
      t->context.ra = (uint64)forkret;
      t->context.sp = t->kstack + PGSIZE;
      return t;
    }
    else
    {
      release(&t->t_lock);
      release(&p->lock);
    }
  }
  return 0;
}
// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;
  struct kthread *t = mykthread();
  // struct cpu *c = mycpu();
  struct proc *p = myproc();
  // Still holding p->lock from scheduler.
  release(&t->t_lock);
  if (holding(&p->lock))
    release(&p->lock);
  // release(&myproc()->lock);
  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}
void freekthread(struct kthread *t)
{
  if (t != 0)
  {
    t->tid = 0;
    t->chan = 0;
    t->killed = 0;
    t->xstate = 0;
    t->trapframe = 0;
    t->state = T_UNUSED;
  }
}
