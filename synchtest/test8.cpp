/* The following test code are copied from http://www.linuxquestions.org/questions/linux-general-1/difference-between-condition-variable-and-mutex-321523/
 */

#include <pthread.h>
#include <stdio.h>

pthread_mutex_t  thread_flag_mutex;

void *thread_function1 (void *thread_arg) {
	pthread_mutex_lock (&thread_flag_mutex);
	fprintf(stderr, "thread_function1\n");
	pthread_mutex_unlock (&thread_flag_mutex);
	return NULL;
}

void *thread_function2 (void *thread_arg) {
	pthread_mutex_lock (&thread_flag_mutex);
	fprintf(stderr, "thread_function2\n");
	pthread_mutex_unlock (&thread_flag_mutex);
	return NULL;
}

int main(int argc, char **argv)
{
	int                   rc=0;
	int                   i;
	pthread_t             threadid[2];

	//pthread_mutex_init (&thread_flag_mutex, NULL);
	rc = pthread_create(&threadid[0], NULL, thread_function1, NULL);
	rc = pthread_create(&threadid[1], NULL, thread_function2, NULL);

	for (i=0; i<2; ++i) {
		rc = pthread_join(threadid[i], NULL);
		printf("pthread_join() in %d\n", i);
	}
	printf("pthread_join() finished\n");
}
