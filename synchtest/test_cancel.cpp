#include<unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *thread_function1 (void *thread_arg) {
	fprintf(stderr, "in the beginning of function1\n");	
	sleep(random()%100);

	return NULL;
}

int main(int argc, char **argv)
{
	int                   rc=0;
	pthread_t             threadid[2];
	srandom(time(NULL));

	rc = pthread_create(&threadid[0], NULL, thread_function1, NULL);
	usleep(1);
	fprintf(stderr, "Issue cancel in main\n");
	pthread_cancel(threadid[0]);
	fprintf(stderr, "Finish cancel in main\n");
	sleep(5);
}
