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

	int * ptr;

	ptr = (int *)malloc(10);

	free(ptr);
  return 0;
}


