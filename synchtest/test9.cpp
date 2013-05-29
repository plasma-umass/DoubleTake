/* The following test code are copied from http://www.linuxquestions.org/questions/linux-general-1/difference-between-condition-variable-and-mutex-321523/
 */

#include <pthread.h>
#include <stdio.h>
int a = 0;
int b = 0;
int c = 0;

void *thread_function1 (void *thread_arg) {
	//usleep(random()%100);

	if(b == 0 && c ==0) {
		a++;
		fprintf(stderr, "first thread increment a to %d (on %p)\n", a, &a);
	}
	return NULL;
}

void *thread_function2 (void *thread_arg) {
//	usleep(random()%100);

	if(a == 0 && c ==0)
		b++;
	return NULL;
}

void *thread_function3 (void *thread_arg) {
//	usleep(random()%100);

	if(a == 0 && b ==0)
		c++;
	return NULL;
}

int main(int argc, char **argv)
{
	int                   rc=0;
	int                   i;
	pthread_t             threadid[2];

	rc = pthread_create(&threadid[0], NULL, thread_function1, NULL);
	rc = pthread_create(&threadid[1], NULL, thread_function2, NULL);
	rc = pthread_create(&threadid[1], NULL, thread_function3, NULL);

	for (i=0; i<2; ++i) {
		rc = pthread_join(threadid[i], NULL);
		printf("pthread_join() in %d\n", i);
	}
	printf("pthread_join() finished.\n");
//	if(a+b+c != 1) {
		printf("a = %d, b = %d, c = %d\n", a, b, c);
//		exit(-1);
//	}
//	sleep(random()%10);
}
