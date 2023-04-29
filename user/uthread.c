#include "types.h"
#include "uthread.h"
// struct uthread {
//     char                ustack[STACK_SIZE];  // the thread's stack
//     enum tstate         state;          // FREE, RUNNING, RUNNABLE
//     struct context      context;        // uswtch() here to run process
//     enum sched_priority priority;       // scheduling priority
// };

// TODO: Check if the thread array table should be handled here (practical sessions say it should be in kernel)
struct uthread threads[MAX_UTHREADS];  // array of all threads
static int current_thread = 0;                // index of currently running thread

// Global array to keep track of the state of each thread
static enum tstate thread_states[MAX_UTHREADS];

// Global array to store the context of each thread
static struct context thread_contexts[MAX_UTHREADS];

// Global array to store the priority of each thread
static enum sched_priority thread_priorities[MAX_UTHREADS];

int uthread_create(void (*start_func)(), enum sched_priority priority) {
    int i;
    for (i = 0; i < MAX_UTHREADS; i++) {
        if (threads[i].state == FREE) {
            // initialize the new thread
            threads[i].state = RUNNABLE;
            threads[i].priority = priority;

            // set the stack pointer to the top of the stack
            threads[i].context.sp = (uint64) &threads[i].ustack[STACK_SIZE];

            // set the return address to the thread exit function
            threads[i].context.ra = (uint64) &uthread_exit;

            // set the start function as the next instruction to be executed
            threads[i].context.sp -= sizeof(uint64);
            *((uint64*) threads[i].context.sp) = (uint64) start_func;

            return 0;  // success
        }
    }
    return -1;  // failure
}

void uthread_yield() {
    int i, next_thread;

    // find the next highest priority runnable thread
    next_thread = -1;
    for (i = 1; i <= MAX_UTHREADS; i++) {
        int index = (current_thread + i) % MAX_UTHREADS;
        if (threads[index].state == RUNNABLE &&
            (next_thread == -1 || threads[index].priority < threads[next_thread].priority)) {
            next_thread = index;
        }
    }

    if (next_thread != -1 && next_thread != current_thread) {
        // save the current thread's context and switch to the next thread
        uswtch(&threads[current_thread].context, &threads[next_thread].context);
        current_thread = next_thread;
    }
}

void uthread_exit() {
    thread_states[current_thread] = FREE; // Mark the current thread as free
    int i;
    for (i = 0; i < MAX_UTHREADS; i++) {
        if (thread_states[i] == RUNNING) { // Find another running thread
            current_thread = i;
            uswtch(&thread_contexts[i], 0); // Switch to the new thread
        }
    }
    exit(0); // If no other running thread found, exit the process
}

int uthread_start_all() {
    // TODO: Implement
    return 0;
}

enum sched_priority uthread_set_priority(enum sched_priority priority) {
    int old_priority = thread_priorities[current_thread];
    thread_priorities[current_thread] = priority; // Set the new priority
    return old_priority;
}

enum sched_priority uthread_get_priority() {
    return thread_priorities[current_thread];
}



struct uthread* uthread_self() {
    // TODO: Implement
    return 0;
}
