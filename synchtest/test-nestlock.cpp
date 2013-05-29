/* The following test code are copied from http://www.linuxquestions.org/questions/linux-general-1/difference-between-condition-variable-and-mutex-321523/
 */

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

pthread_mutex_t  mutex1;
pthread_mutex_t  mutex2;

void *thread_function1 (void *thread_arg) {

  pthread_mutex_lock (&mutex1);
  write(2, "inside mutex1 only\n", 19);
  pthread_mutex_lock (&mutex2);
  write(2, "inside mutex1 and mutex2\n", 28);
  pthread_mutex_unlock (&mutex2);
  pthread_mutex_unlock (&mutex1);
	return NULL;
}

int main(int argc, char **argv)
{
	int                   rc=0;
	int                   i;
	pthread_t             threadid[2];

	pthread_mutex_init(&mutex1, NULL);
	pthread_mutex_init(&mutex2, NULL);
	rc = pthread_create(&threadid[0], NULL, thread_function1, NULL);
  rc = pthread_join(threadid[0], NULL);
	fprintf(stderr, "after pthread_join() finished\n");
}
