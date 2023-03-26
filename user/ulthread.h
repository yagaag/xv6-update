#ifndef __UTHREAD_H__
#define __UTHREAD_H__

#include <stdbool.h>

#define MAXULTHREADS 100

enum ulthread_state {
  FREE,
  RUNNABLE,
  YIELD,
};

enum ulthread_scheduling_algorithm {
  ROUNDROBIN,   
  PRIORITY,     
  FCFS,         // first-come-first serve
};

struct ulthread_context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

struct ulthread {
  int tid;                     // Thread ID
  enum ulthread_state state;   // User-level thread state
  uint64 *stack;               // Virtual address of stack
  struct ulthread_context context; // swtch() here to run thread
  uint64 *start_func;           // The start function of the thread
  char name[16];               // Thread name (debugging)
  int priority;                // Priority of the thread
  int reached_at;              // Thread creation time for FCFS
};

struct ulthread_list {
  int total;                  // total number of threads currently
  int so_far;                 // total number of threads ever
  int current;                // index of the current thread
  struct ulthread threads[MAXULTHREADS]; // array of threads
  enum ulthread_scheduling_algorithm algorithm; // The scheduling algorithm to use
};

#endif