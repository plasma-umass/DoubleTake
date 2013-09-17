#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

int flags[10];
pthread_cond_t * theCond;
pthread_mutex_t * theMutex;

extern "C" void * consumer (void *) {
  for (int i = 0; i < 10; i++) {
    pthread_mutex_lock (theMutex);
	while(flags[i] != 1) {
    	pthread_cond_wait (theCond, theMutex);
	}
    fprintf (stderr, "<<consumed %d>>\n", i);
    pthread_mutex_unlock (theMutex);
  }
  fprintf(stderr, "%d: consumer exit\n", getpid());
  return NULL;
}

extern "C" void * producer (void *) {
  for (int i = 0; i < 10; i++) {
    pthread_mutex_lock (theMutex);
	flags[i] = 1;
    pthread_cond_signal (theCond);
    pthread_mutex_unlock (theMutex);
    fprintf (stderr, "<<produced %d>>\n", i);
  }

  fprintf(stderr, "%d: producer exit\n", getpid());
}

int main (int argc, char * argv[])
{
  theCond = new pthread_cond_t;
  theMutex = new pthread_mutex_t;
  pthread_mutex_init (theMutex, NULL);
  pthread_cond_init (theCond, NULL);

  pthread_t threads[2];

  for(int i = 0; i< 10; i++) {
	flags[i] = 0;
  }

  pthread_create (&threads[0], NULL, consumer, NULL);
  pthread_create (&threads[1], NULL, producer, NULL);

  for (int i = 0; i < 2; i++) {
    pthread_join (threads[i], NULL);
  }

  return 0;
}
