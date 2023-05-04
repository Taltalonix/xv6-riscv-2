#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];
struct spinlock tid_lock;
extern void forkret(void);
void kthreadinit(struct proc *p)
{

  struct kthread *thread;
  initlock(&tid_lock, "nexttid");
  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->lock, "threadlock");
    // todo: check if need to aquire locks : wait lock → proc lock → thread lock
    //  do i need to add wait_lock obj to this file as in proc.c
    acquire(&kt->lock);
    kt->state = UNUSED;
    kt->parent = p;
    release(&kt->lock);

    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}

struct kthread *mykthread()
{
  return &mycpu()->kthread;
  // return &myproc()->kthread[0];
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
  // todo : check which lock to lock hear
  // aquire(&p->lock);
  acquire(&p->counter_lock);
  acquire(&tid_lock);
  tid = p->counter;
  p->counter++;
  release(&tid_lock);
  release(&p->counter_lock);
  // release(&p->lock);
  return tid;
}

static struct kthread *allockthread(struct proc *p)
{
  struct kthread *t;
  // acquire(&p->lock); - check if the lock is allready aquired in allocproc
  for (int i = 0; i < NKT; i++)
  {
    if (p->kthread[i].state == UNUSED)
    {
      t->tid = alloctid(p);
      t->state = USED;
      t->trapframe = get_kthread_trapframe(p, t);
      memset(&t->context, 0, sizeof(t->context));
      t->context.ra = (uint64)forkret;
      t->context.sp = p->kthread->kstack + PGSIZE;
      // p->kthread->trapframe = get_kthread_trapframe(p, p->kthread);
      // t->trapframe = get_kthread_trapframe(p, t);
      acquire(&t->lock);
      return t;
    }
  }
}

static void freekthread(struct kthread *t)
{
  if (t->trapframe)
  {

    kfree((void *)t->trapframe);
    // todo - check if we need to free the context
    t->trapframe = 0;
    t->chan = 0;
    t->killed = 0;
    t->kstack = 0;
    t->parent = 0;
    t->tid = 0;
    t->xstate = 0;
    t->state = UNUSED;
  }
}
