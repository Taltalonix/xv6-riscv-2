#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "uthread.h"

// TODO: Check if the thread array table should be handled here (practical sessions say it should be in kernel)
struct uthread threads[MAX_UTHREADS]; // array of all threads
struct uthread *current_thread;
int nums[MAX_UTHREADS]; // index of currently running thread
// Global array to keep track of the state of each thread
// static enum tstate thread_states[MAX_UTHREADS];

// // Global array to store the context of each thread
// static struct context thread_contexts[MAX_UTHREADS];

// // Global array to store the priority of each thread
// static enum sched_priority thread_priorities[MAX_UTHREADS];

static int firstTime = 1;
static int firstInit = 1;

void initTable(void)
{
    if (firstInit)
    {
        for (int i = 0; i < MAX_UTHREADS; i++)
        {
            threads[i].state = FREE;
            nums[i] = 0;
        }
        current_thread = malloc(sizeof(threads));
        firstInit = 0;
    }
}
int getMinRunnablePriority(int priority)
{
    int min = 1000000;
    int idx = -1;
    for (int i = 0; i < MAX_UTHREADS; i++)
    {
        if (threads[i].state == RUNNABLE && threads[i].priority == priority && nums[i] < min)
        {
            min = nums[i];
            idx = i;
        }
    }
    return idx;
}
int getMaxPriority(void)
{
    int med = 0;
    for (int i = 0; i < MAX_UTHREADS; i++)
    {
        if (threads[i].state == RUNNABLE)
        {
            if (threads[i].priority == HIGH)
            {
                return 2;
            }
            if (threads[i].priority == MEDIUM)
            {
                med = 1;
            }
        }
    }
    return med;
}
int getMaxRunnablePriority(void)
{
    int maxPriority = getMaxPriority();
    int pos = -1;
    switch (maxPriority)
    {
    case 0:
        pos = getMinRunnablePriority(0);
        if (pos == -1)
        {
            return pos;
        }
        nums[pos]++;
        return pos;
        break;
    case 1:
        pos = getMinRunnablePriority(1);
        if (pos == -1)
        {
            return pos;
        }
        nums[pos]++;
        return pos;
        break;
    case 2:
        pos = getMinRunnablePriority(2);
        if (pos == -1)
        {
            return pos;
        }
        nums[pos]++;
        return pos;
        break;
    default:
        return pos;
        break;
    }
    // int pos = -1;
    // int med = 0;
    // int i;
    // for (i = 0; i < MAX_UTHREADS; i++)
    // {
    //     if (threads[i].state == RUNNABLE)
    //     {
    //         if (threads[i].priority == HIGH)
    //         {
    //             return i;
    //         }
    //         else if (threads[i].priority == MEDIUM)
    //         {
    //             med = 1;
    //             pos = i;
    //         }
    //         else if (!med && threads[i].priority == LOW)
    //         {
    //             pos = i;
    //         }
    //     }
    // }
    // return pos;
}

int uthread_create(void (*start_func)(), enum sched_priority priority)
{

    int i;
    initTable();
    for (i = 0; i < MAX_UTHREADS; i++)
    {
        if (threads[i].state == FREE)
        {
            // initialize the new thread
            threads[i].priority = priority;
            memset(&threads[i].context, 0, sizeof(threads[i].context));
            // set the stack pointer to the top of the stack
            threads[i].context.sp = (uint64)&threads[i].ustack[STACK_SIZE];

            // set the return address to the thread exit function
            threads[i].context.ra = (uint64)start_func;

            // set the start function as the next instruction to be executed
            // threads[i].context.sp -= sizeof(uint64);
            // *((uint64 *)threads[i].context.sp) = (uint64)start_func;

            threads[i].state = RUNNABLE;

            return 0; // success
        }
    }
    return -1; // failure
}

void uthread_yield()
{

    // find the next highest priority runnable thread
    struct uthread *run;
    current_thread->state = RUNNABLE;
    run = &threads[getMaxRunnablePriority()];
    // save the current thread's context and switch to the next thread
    run->state = RUNNING;
    current_thread = run;
    uswtch(&current_thread->context, &run->context);
}

void uthread_exit()
{
    current_thread->state = FREE; // Mark the current thread as free
    struct uthread *tmp;
    int pos = getMaxRunnablePriority();
    if (pos != -1)
    {
        struct uthread *thread_to_run = &threads[pos];
        tmp = current_thread;
        current_thread = thread_to_run;
        thread_to_run->state = RUNNING;
        tmp->state = FREE;
        uswtch(&tmp->context, &thread_to_run->context);
    }
    exit(0); // If no other running thread found, exit the process
}

int uthread_start_all()
{
    if (firstTime)
    {
        struct uthread *run;
        int pos = getMaxRunnablePriority();
        if (pos != -1)
        {
            run = &threads[pos];
            run->state = RUNNING;
            firstTime = 0;
            uswtch(&current_thread->context, &run->context);
        }
    }
    // if reached to hear this is an error
    return -1;
}

enum sched_priority uthread_set_priority(enum sched_priority priority)
{
    int old_priority = current_thread->priority;
    current_thread->priority = priority; // Set the new priority
    return old_priority;
}

enum sched_priority uthread_get_priority()
{
    return current_thread->priority;
}

struct uthread *uthread_self()
{
    // todo - Check if this is the intention
    return current_thread;
}
