#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "kthread.h"

extern struct proc proc[NPROC];

struct spinlock thread_wait_lock;

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

// 2.3
int kthread_create(void *(*start_func)(), uint64 stack, uint stack_size)
{
  // TODO: Implement 2.3
  int tid;

  struct proc *parent_proc = myproc();

  struct kthread *kt = mykthread();
  struct kthread *nkt;

  // Allocate thread.
  if ((nkt = allockthread(parent_proc)) == 0)
  {
    return -1;
  }

  // Copy user memory from parent to child.
  // if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  // {
  //   freeproc(np);
  //   release(&np->lock);
  //   return -1;
  // }
  // np->sz = p->sz;

  // copy saved user registers.
  *(nkt->trapframe) = *(kt->trapframe);

  // Cause fork to return 0 in the child.
  nkt->trapframe->a0 = 0;

  // // increment reference counts on open file descriptors.
  // for (i = 0; i < NOFILE; i++)
  //   if (p->ofile[i])
  //     np->ofile[i] = filedup(p->ofile[i]);
  // np->cwd = idup(p->cwd);

  // safestrcpy(np->name, p->name, sizeof(p->name));

  // pid = np->pid;
  tid = nkt->tid;

  release(&nkt->t_lock);
  // release(&np->lock);

  acquire(&thread_wait_lock);
  // np->parent = p;
  nkt->parent = parent_proc;
  release(&thread_wait_lock);

  // acquire(&np->lock);
  // np->state = USED;
  acquire(&nkt->t_lock);
  nkt->state = T_RUNNABLE;
  release(&nkt->t_lock);
  // release(&np->lock);

  return tid;
}

int kthread_id()
{
  return mykthread()->tid;
}

int kthread_kill(int ktid)
{
  // TODO: Check if ok
  struct kthread *kt = mykthread();
  struct proc *p = myproc();

  // find the kthread with the given ktid
  acquire(&p->lock);
  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    if (kt->tid == ktid)
    {
      break;
    }
  }
  if (kt == &p->kthread[NKT])
  {
    release(&p->lock);
    return -1; // kthread with the given ktid not found
  }
  acquire(&kt->t_lock);
  kt->killed = 1;
  if (kt->state == T_SLEEPING)
  {
    kt->state = T_ZOMBIE;
    release(&kt->t_lock);
    release(&p->lock);
  }
  else
  {
    // the kthread is not sleeping, so we need to call kthread_exit ourselves
    release(&kt->t_lock);
    release(&p->lock);
    kthread_exit(-1);
  }
  return 0;
}

void kthread_exit(int status)
{
  struct proc *p = myproc();
  struct kthread *t = mykthread();

  acquire(&p->lock);
  acquire(&t->t_lock);

  t->xstate = status;
  t->state = T_ZOMBIE;

  // Wake process waiting on this thread to exit.
  wakeup(t);

  // Release lock on the thread and the process.
  release(&t->t_lock);
  release(&p->lock);

  // Jump into the scheduler, never to return.
  sched();
}

int kthread_join(int ktid, int *status)
{
  // TODO: Check if ok
  struct proc *p = myproc();
  struct kthread *t;

  acquire(&p->lock);

  for (;;)
  {
    // Scan through the thread table looking for the specified thread.
    int found = 0;
    for (t = p->kthread; t < &p->kthread[NKT]; t++)
    {
      if (t->tid == ktid)
      {
        found = 1;
        // Found the thread, wait for it to exit.
        while (t->state != T_ZOMBIE)
        {
          if (t->state == T_UNUSED || t->killed)
          {
            release(&p->lock);
            return -1;
          }
          sleep(t, &p->lock);
        }
        // Thread has exited, copy the exit status if requested.
        if (*status != -1)
        {
          *status = t->xstate;
        }
        freekthread(t);
        release(&p->lock);
        return 0;
      }
    }
    // The specified thread wasn't found in the thread table.
    if (!found)
    {
      release(&p->lock);
      return -1;
    }
    // The specified thread hasn't exited yet.
    sleep(p, &p->lock);
  }
}
