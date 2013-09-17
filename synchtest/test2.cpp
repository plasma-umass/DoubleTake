#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <stdio.h>

/* For safe condition variable usage, must use a boolean predicate and  */
/* a mutex with the condition.                                          */
int                 waiter = 0;
int                 broadcast = 0;
pthread_cond_t      cond1;
pthread_cond_t      cond2;
pthread_mutex_t     mutex;

#define NTHREADS    1

void *threadfunc(void *parm)
{
  int           rc;
  int  pid = (intptr_t)parm;
  fprintf(stderr, "CHILD %d\n", pid);
  rc = pthread_mutex_lock(&mutex);
  waiter++;
  fprintf(stderr, "pid %d waiter %d\n", pid, waiter);
  if(waiter == NTHREADS) {
	  broadcast = 1;
	  pthread_cond_signal(&cond2);
    fprintf(stderr, "%d send out signal and finished.\n", pid);
  }

  rc = pthread_cond_wait(&cond1, &mutex);
  printf("pthread_cond_wait() in %d\n", pid);
  rc = pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(int argc, char **argv)
{
  int                   rc=0;
  int                   i;
  pthread_t             threadid[NTHREADS];

  printf("Create %d threads. cond size is %d\n", NTHREADS, sizeof(pthread_cond_t));
  printf("broadcast %p, waiter %p\n", &broadcast, &waiter);
  pthread_cond_init(&cond1, NULL);
  pthread_cond_init(&cond2, NULL);
  pthread_mutex_init(&mutex, NULL);

  for(i=0; i<NTHREADS; ++i) {
    rc = pthread_create(&threadid[i], NULL, threadfunc, (void *)i);
  }

  rc = pthread_mutex_lock(&mutex);

  /* The condition has occured. Set the flag and wake up any waiters */
  while(broadcast != 1) {
  	fprintf(stderr, "Main thread is waiting...\n");
	  pthread_cond_wait(&cond2, &mutex);
  	fprintf(stderr, "Main thread is waken up...\n");
  }
  fprintf(stderr, "Wake up all waiters...\n");
  rc = pthread_cond_broadcast(&cond1);
  fprintf(stderr, "pthread_cond_broadcast()\n", rc);

  rc = pthread_mutex_unlock(&mutex);

  printf("Wait for threads and cleanup\n");
  for (i=0; i<NTHREADS; ++i) {
    rc = pthread_join(threadid[i], NULL);
    printf("pthread_join() in %d\n", i);
  }
  printf("pthread_join() finshed\n");
  pthread_cond_destroy(&cond1);
  pthread_cond_destroy(&cond2);
  pthread_mutex_destroy(&mutex);

  printf("Main completed\n");
  return 0;
}


