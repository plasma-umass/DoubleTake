#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* For safe condition variable usage, must use a boolean predicate and  */
/* a mutex with the condition.                                          */
int        x = 0;
int        y = 0;

void *threadfunc1(void *parm)
{
  unsigned long index = (unsigned long)parm; 
  fprintf(stderr, "*************thread %d beginning\n", index, x, y);
  x = 1;
  y = 0;
  fprintf(stderr, "*************thread %d: x = %d, y = %d\n", index, x, y);
}

void *threadfunc2(void *parm)
{
  unsigned long index = (unsigned long)parm; 
  fprintf(stderr, "*************thread %d beginning\n", index, x, y);
  y = 1;
  x = 0;
  fprintf(stderr, "*************thread %d: x = %d, y = %d\n", index, x, y);
}

int main(int argc, char **argv)
{
  int                   rc=0;
  int                   i;
  pthread_t             threadid[2];

  fprintf(stderr, "IN the beginning of MAIN, x %d(at %p) y %d(at %p)\n", x, &x, y, &y); 
  write(2, "create thread1**\n", 17);
  pthread_create(&threadid[0], NULL, threadfunc1, (void *)1);

  write(2, "create thread2**\n", 17);
  pthread_create(&threadid[1], NULL, threadfunc2, (void *)2);

  write(2, "Before pthread_join\n", 20);
  fprintf(stderr, "MAIN before join x %d y %d\n", x, y);
  //sleep(2); 
  rc = pthread_join(threadid[0], NULL);
  rc = pthread_join(threadid[1], NULL);

  fprintf(stderr, "After pthread_join\n"); 
  //sleep(5); 
  if(x == 1 && y == 1) {
    printf("pthread_join() finshed. x = %d, y = %d\n", x, y);
  }
  fprintf(stderr, "pthread_join() finshed. x = %d, y = %d\n", x, y);
  return 0;
}


