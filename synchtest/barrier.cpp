#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <stdlib.h>

/*
 We are going to design a weird usage of barrier.
 For example, we set the barrier to only half of existing threads.
 Then we want to guarantee the order to pass the barrier.
 */
#define NTHREADS 3
#define TOTAL_THREADS (NTHREADS * 2)

pthread_barrier_t gbarrier;
extern void callAtomicEnd(void);

void * threadfunc(void * ptr) {
  unsigned long tindex = (unsigned long)ptr;

  if(tindex < NTHREADS) {
    usleep(random()%(NTHREADS*TOTAL_THREADS));
  }
  else {
    usleep(random()%NTHREADS);
  }

  pthread_barrier_wait(&gbarrier);

  fprintf(stderr, "thread %d passed the barrier\n", tindex);
  return NULL;
}

void exittest(void) {
  int test;
  fprintf(stderr, "in the exitting~~~~~~~~~ test is at %p\n", &test);
}

int main()
{
  int rc = 0;  
  int                   i;
  pthread_t             threadid[TOTAL_THREADS];

  atexit(exittest);
  srandom(time(NULL));
  pthread_barrier_init(&gbarrier, NULL, NTHREADS);
  
  for(i=0; i< TOTAL_THREADS; ++i) {
    rc = pthread_create(&threadid[i], NULL, threadfunc, (void *)i);
  }

  for (i=0; i<TOTAL_THREADS; ++i) {
    rc = pthread_join(threadid[i], NULL);
  }
  
  pthread_barrier_destroy(&gbarrier);
//  callAtomicEnd();
}
