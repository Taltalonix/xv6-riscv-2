#include "types.h"
#include "uthread.h"

// TODO: Check if the thread array table should be handled here (practical sessions say it should be in kernel)
struct uthread threads[MAX_UTHREADS]; // array of all threads
struct uthread *current_thread;       // index of currently running thread

// Global array to keep track of the state of each thread
static enum tstate thread_states[MAX_UTHREADS];

// Global array to store the context of each thread
static struct context thread_contexts[MAX_UTHREADS];

// Global array to store the priority of each thread
static enum sched_priority thread_priorities[MAX_UTHREADS];

static int firstTime = 1;

void initTable(void)
{
    if (firstTime)
    {
        for (int i = 0; i < MAX_UTHREADS; i++)
        {
            threads[i].state = FREE;
        }
        current_thread = malloc(sizeof(threads));
        firstTime = 0;
    }
}
int getMaxRunnablePriority(void)
{
    int pos = -1;
    int med, i;
    for (i = 0; i < MAX_UTHREADS; i++)
    {
        if (threads[i].state == RUNNABLE && &threads[i] != current_thread)
        {
            if (threads[i].priority == HIGH)
            {
                return i;
            }
            else if (threads[i].priority == MEDIUM)
            {
                med = 1;
                pos = i;
            }
            if (!med)
            {
                pos = i;
            }
        }
    }
    return pos;
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

            // set the stack pointer to the top of the stack
            threads[i].context.sp = (uint64)&threads[i].ustack[STACK_SIZE];

            // set the return address to the thread exit function
            threads[i].context.ra = (uint64)&uthread_exit;

            // set the start function as the next instruction to be executed
            threads[i].context.sp -= sizeof(uint64);
            *((uint64 *)threads[i].context.sp) = (uint64)start_func;

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
    // struct uthread *next_thread;
    //  int max_priority = 0;
    //  for (next_thread = threads; next_thread < &threads[MAX_UTHREADS]; next_thread++)
    //  {

    //     if (next_thread != -1 && next_thread != current_thread && next_thread->priority >= max_priority && next_thread->state == RUNNABLE)
    //     {
    //         run = next_thread;
    //         max_priority = next_thread->priority;
    //     }
    // }
    run = &threads[getMaxRunnablePriority()];
    // save the current thread's context and switch to the next thread
    run->state == RUNNING;
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
        // for (i = 0; i < MAX_UTHREADS; i++)
        // {
        //     if (thread_states[i] == RUNNING)
        //     {
        //         // Find another running thread
        //         current_thread = i;
        //         uswtch(&thread_contexts[i], 0); // Switch to the new thread
        //     }
        // }
        tmp = current_thread;
        current_thread = thread_to_run;
        thread_to_run->state = RUNNING;
        uswtch(&tmp->context, &thread_to_run->context);
    }
    exit(0); // If no other running thread found, exit the process
}

int uthread_start_all()
{
    firstTime = 1;
    if (firstTime)
    {
        uthread_yield();
        firstTime = 0;
        return -1;
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
