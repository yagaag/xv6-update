#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

#include "user/ulthread.h"
#include <stdarg.h>

/* Stack region for different threads */
char stacks[PGSIZE*MAXULTHREADS];
int t_count = 0;

void ul_start_func(void) {
    if (t_count < 15) {
        uint64 args[6] = {0,0,0,0,0,0};
        ulthread_create((uint64) ul_start_func, (uint64) (stacks + PGSIZE + t_count*(PGSIZE)), args, -1);
        t_count++;
    }
    ulthread_destroy();
}

int
main(int argc, char *argv[])
{
    /* Clear the stack region */
    memset(&stacks, 0, sizeof(stacks));

    /* Initialize the user-level threading library */
    ulthread_init(FCFS);

    printf("Testing first-come-first-serve scheduling:\n");
    printf("Thread 1 should execute once and get destroyed\n");
    printf("Thread 2 should be scheduled twice before getting destroyed\n");
    printf("Thread 8 should be scheduled eight times before yielding at the end\n");

    /* Create a user-level thread */
    uint64 args[6] = {0,0,0,0,0,0};  
    for (int i=0; i<5; i++) {
        ulthread_create((uint64) ul_start_func, (uint64) (stacks + PGSIZE + i*(PGSIZE)), args, -1);
        t_count++;
    }

    /* Schedule some of the threads */
    ulthread_schedule();

    printf("[*] First-Come-First-Serve Scheduling Test Complete.\n");
    return 0;
}