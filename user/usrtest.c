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

void test1()
{
    for (int i = 0; i < 3; i++)
    {
        printf("[INFO] T1: %d\n", i);
        uthread_yield();
    }
    printf("[INFO] T1 END\n");

    uthread_exit();
}
void test2()
{
    for (int i = 0; i < 3; i++)
    {
        printf("[INFO] T2: %d\n", i);
        uthread_yield();
    }
    printf("[INFO] T2 END\n");
    uthread_exit();
}
void test3()
{
    for (int i = 0; i < 3; i++)
    {
        printf("[INFO] T3: %d\n", i);
        uthread_yield();
    }
    printf("[INFO] T3 END\n");

    uthread_exit();
}

int main()
{
    uthread_create(test1, 1);
    uthread_create(test2, 2);
    uthread_create(test3, 1);
    uthread_start_all();
    exit(0);
}