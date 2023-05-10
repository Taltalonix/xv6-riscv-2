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

struct uthread threads[MAX_UTHREADS]; // array of all threads
struct uthread *current_thread;
int round_flags[MAX_UTHREADS]; // index of currently running thread
// Global array to keep track of the state of each thread
// static enum tstate thread_states[MAX_UTHREADS];

// // Global array to store the context of each thread
// static struct context thread_contexts[MAX_UTHREADS];

// // Global array to store the priority of each thread
// static enum sched_priority thread_priorities[MAX_UTHREADS];

static int is_first_time = 1;
static int is_first_init = 1;

// ---------------------------------- DEBUG START ----------------------------------
void DEBUG_print_separator()
{
    printf("[DEBUG] ----------------------------------------------------------------\n");
}

void DEBUG_print_thread(struct uthread *t, int index)
{
    printf("\t{ State: %s, Priority: %d, RoundFlag: %d }",
           t->state == RUNNING
               ? "RUNNING"
           : t->state == RUNNABLE
               ? "RUNNABLE"
               : "FREE",
           t->priority,
           round_flags[index]);
}
void DEBUG_print_threads()
{
    int i;
    printf("[DEBUG] Thread Table: [\n");
    for (i = 0; i < MAX_UTHREADS; i++)
    {
        DEBUG_print_thread(&threads[i], i);
        if (i < MAX_UTHREADS - 1)
        {
            printf(",");
        }
        printf("\n");
    }
    printf("]\n\n");
}
// ---------------------------------- DEBUG END ----------------------------------

void init_table(void)
{
    if (is_first_init)
    {
        for (int i = 0; i < MAX_UTHREADS; i++)
        {
            threads[i].state = FREE;
            round_flags[i] = 0;
        }
        current_thread = malloc(sizeof(threads));
        is_first_init = 0;
    }
}
int get_first_index(int *found_runnable)
{
    int first_index = -1;

    struct uthread *ithread;
    *found_runnable = 0;
    int i;
    for (i = 0; i < MAX_UTHREADS; i++)
    {
        ithread = &threads[i];
        if (ithread->state == RUNNABLE)
        {
            *found_runnable = 1;
            if (!round_flags[i] && (first_index == -1 || ithread->priority > threads[first_index].priority))
            {
                first_index = i;

                // No need to continue
                // if(first_priority == HIGH) {
                //     break;
                // }
            }
        }
    }
    return first_index;
}
int get_max_runnable_priority(void)
{
    int found_runable = 0;
    int first_index = get_first_index(&found_runable);
    // DEBUG_print_separator();
    // printf("[DEBUG] first_index: %d\n", first_index);
    // printf("[DEBUG] found_runable: %d\n", found_runable);
    // DEBUG_print_separator();
    int i;

    if (first_index != -1)
    {
        round_flags[first_index] = 1;
    }
    if (found_runable && first_index == -1 /* Round Over, restart */)
    {
        // DEBUG_print_separator();
        // DEBUG_print_separator();
        // DEBUG_print_threads();
        for (i = 0; i < MAX_UTHREADS; i++)
        {
            round_flags[i] = 0;
        }
        first_index = get_first_index(&found_runable);
        // DEBUG_print_threads();
        // printf("[DEBUG] Returning index: %d\n", first_index);
        // DEBUG_print_separator();
        // DEBUG_print_separator();
    }

    return first_index;
}

int uthread_create(void (*start_func)(), enum sched_priority priority)
{

    int i;
    struct uthread *cthread;
    init_table();
    for (i = 0; i < MAX_UTHREADS; i++)
    {
        cthread = &threads[i];
        if (cthread->state == FREE)
        {
            // initialize the new thread
            cthread->priority = priority;
            memset(&cthread->context, 0, sizeof(cthread->context));
            // set the stack pointer to the top of the stack
            cthread->context.sp = (uint64)(cthread->ustack +STACK_SIZE);

            // set the return address to the thread exit function
            cthread->context.ra = (uint64)start_func;

            // set the start function as the next instruction to be executed
            // threads[i].context.sp -= sizeof(uint64);
            // *((uint64 *)threads[i].context.sp) = (uint64)start_func;

            cthread->state = RUNNABLE;

            return 0; // success
        }
    }
    return -1; // failure
}

void uthread_yield()
{

    // find the next highest priority runnable thread
    struct uthread *next_thread;
    current_thread->state = RUNNABLE;
    next_thread = &threads[get_max_runnable_priority()];
    // save the current thread's context and switch to the next thread
    next_thread->state = RUNNING;
    current_thread = next_thread;
    uswtch(&current_thread->context, &next_thread->context);
}

void uthread_exit()
{
    current_thread->state = FREE; // Mark the current thread as free
    struct uthread *tmp;
    int pos = get_max_runnable_priority();
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
    printf("[DEBUG] Starting All...\n");
    // DEBUG_print_threads();
    if (is_first_time)
    {
        struct uthread *run;
        int pos = get_max_runnable_priority();
        if (pos != -1)
        {
            run = &threads[pos];
            run->state = RUNNING;
            is_first_time = 0;
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
    // TOOD: Check if this is the intention
    return current_thread;
}
