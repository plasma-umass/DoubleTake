#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* For safe condition variable usage, must use a boolean predicate and  */
/* a mutex with the condition.                                          */
int                 ready = 0;
pthread_cond_t      cond1;
pthread_mutex_t     mutex;

void *threadfunc(void *parm)
{
  int           rc;
  int  pid = (intptr_t)parm;
  fprintf(stderr, "%d: child in the beginning\n", getpid());
//  printf("thread %d waking from usleep\n", pid);
  rc = pthread_mutex_lock(&mutex);
  fprintf(stderr, "%d: child in the lock with ready %d\n", getpid(), ready);
  while(ready == 0) {
  	fprintf(stderr, "%d: pthread_cond_wait() in %d\n", getpid(), pid);
  	rc = pthread_cond_wait(&cond1, &mutex);
  	fprintf(stderr, "wake up after pthread_cond_wait\n");
  }
  fprintf(stderr, "%d: child unlock with ready %d\n", getpid(), ready);
  rc = pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(int argc, char **argv)
{
  int                   rc=0;
  int                   i;
  pthread_t             threadid[1];
  void * ptr;

  fprintf(stderr, "main %d\n", getpid());

  ptr = malloc(5);
  fprintf(stderr, "main %d 111 with ptr %p\n", getpid(), ptr);
  pthread_cond_init(&cond1, NULL);
  fprintf(stderr, "main %d 111\n", getpid());
  pthread_mutex_init(&mutex, NULL);

  fprintf(stderr, "main %d after mutex\n", getpid());
  pthread_create(&threadid[0], NULL, threadfunc, (void *)0);

  fprintf(stderr, "main %d after thread create\n", getpid());
  
  sleep(1);

  fprintf(stderr, "%d : main to get write lock\n", getpid());

  rc = pthread_mutex_lock(&mutex);

  ready = 1;
  fprintf(stderr, "main %d in lock and ready is %d\n", getpid(), ready);
  /* The condition has occured. Set the flag and wake up any waiters */
  rc = pthread_cond_broadcast(&cond1);

  fprintf(stderr, "main %d in lock and ready is %d. Try to unlock\n", getpid(), ready);
  rc = pthread_mutex_unlock(&mutex);

  fprintf(stderr, "%d: JOOOOOOOOOOOOOOIN\n", getpid());
  rc = pthread_join(threadid[0], NULL);
  printf("pthread_join() finshed\n");

  printf("Main completed\n");
  return 0;
}


