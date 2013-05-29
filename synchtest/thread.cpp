#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NTHREADS 3

/* For safe condition variable usage, must use a boolean predicate and  */
/* a mutex with the condition.                                          */
int        x = 0;
int        y = 0;

void *threadfunc1(void *parm)
{
  unsigned long index = (unsigned long)parm; 
  fprintf(stderr, "*************thread %d beginning, thread %p\n", index, pthread_self());
  fprintf(stderr, "*************thread %d: x = %d, y = %d\n", index, x, y);
}

int main(int argc, char **argv)
{
  int                   rc=0;
  int                   i;
  pthread_t             threadid[2];

  fprintf(stderr, "Before the next phase, x %d y %d\n", x, y);
  for(i = 0; i < NTHREADS; i++) {
    pthread_create(&threadid[i], NULL, threadfunc1, (void *)i);
  }
  for(i = 0; i < NTHREADS; i++) {
    pthread_join(threadid[i], NULL);
  }

  x = 0;
  y = 1;
  fprintf(stderr, "Before the next phase, x %d y %d\n", x, y);
  
  for(i = 0; i < NTHREADS; i++) {
    pthread_create(&threadid[i], NULL, threadfunc1, (void *)i);
  }
  for(i = 0; i < NTHREADS; i++) {
    pthread_join(threadid[i], NULL);
  }
  
  x = 1;
  y = 1;
  fprintf(stderr, "Before the next phase, x %d y %d\n", x, y);
  
  for(i = 0; i < NTHREADS; i++) {
    pthread_create(&threadid[i], NULL, threadfunc1, (void *)i);
  }
  for(i = 0; i < NTHREADS; i++) {
    pthread_join(threadid[i], NULL);
  }

  return 0;
}


