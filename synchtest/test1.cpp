/* The following test code are copied from http://www.linuxquestions.org/questions/linux-general-1/difference-between-condition-variable-and-mutex-321523/
 */

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int  thread_flag = 2;
pthread_cond_t  cond;
pthread_mutex_t  mutex;

void do_junk(int size) {
	int i;
	int f,f1,f2;

	f1 =12441331;
	f2 = 3245235;
	for (i = size; i > 0; i--) {
		f *= f1;        /*    x    x     1/x */
		f1 *= f2;       /*    x    1     1/x */
		f1 *= f2;       /*    x    1/x   1/x */
		f2 *= f;        /*    x    1/x   1   */
		f2 *= f;        /*    x    1/x   x   */
		f *= f1;        /*    1    1/x   x   */
		f *= f1;        /*    1/x  1/x   x   */
		f1 *= f2;       /*    1/x  1     x   */
		f1 *= f2;       /*    1/x  x     x   */
		f2 *= f;        /*    1/x  x     1   */
		f2 *= f;        /*    1/x  x     1/x */
		f *= f1;        /*    1    x     1/x */
	}
}

void set_thread_flag(int flag_value) {
	pthread_mutex_lock (&mutex);
	thread_flag=flag_value;
	fprintf(stderr, "SETFLAG from %lx, now thread_flag %d\n", pthread_self(), thread_flag);
	pthread_cond_signal(&cond);
	// nothing can check the flag till this lock is removed
	pthread_mutex_unlock (&mutex);
}

void *thread_function1 (void *thread_arg) {
	{
		fprintf(stderr, "THREAD1, beginning with pid %d\n", pthread_self());
		pthread_mutex_lock (&mutex);
		fprintf(stderr, "THREAD1 %d get lock\n", getpid());
		if (thread_flag != 1) {
			// now we can just hang here 
			// it's important to know this atomically unnlocks the mutex and then when signaled reaquires the lock
			fprintf(stderr, "THREAD1 %d is going to wait\n", getpid());
			pthread_cond_wait(&cond, &mutex);
			fprintf(stderr, "Haha, THREAD1 I am woken up!!!!\n");
		}
		else {
			fprintf(stderr, "Haha, THREAD1 I am ok without waiting!!!!\n");
		}
		// when we get here we know someone set the flag
		pthread_mutex_unlock (&mutex);
	    fprintf(stderr, "THREAD1 pid %d with flag 1, we will do junk\n", getpid());
		do_junk(0x12345);
		// now send the other thread blocked in while (thread_flag) on it's merry way
		set_thread_flag(2);
	}

	fprintf(stderr, "Thread1 %d is exiting with thread_flag %d\n", getpid(), thread_flag);
	return NULL;
}

void *thread_function2 (void *thread_arg) {
    {
		fprintf(stderr, "THREAD2 beginning, pid %d\n", getpid());
		pthread_mutex_lock (&mutex);
		fprintf(stderr, "THREAD2 %d get lock with thread_flag %d\n", getpid(), thread_flag);
		if (thread_flag != 2) {
			// now we can just hang here 
			// it's important to know this atomically unnlocks the mutex and then when signaled reaquires the lock
			fprintf(stderr, "THREAD2 %d is going to wait\n", getpid());
			pthread_cond_wait(&cond, &mutex);
			fprintf(stderr, "Haha, THREAD2 I am woken up!!!!\n");
		}
		// when we get here we know someone set the flag
		pthread_mutex_unlock (&mutex);
	    fprintf(stderr, "THREAD2  pid %d with flag 2, we will do junk\n", getpid());
		do_junk(0x54321);
		// now send the other thread blocked in while (!thread_flag) on it's merry way
		set_thread_flag(1);
	}

	fprintf(stderr, "Thread2 %d is exiting\n", getpid());
	return NULL;
}

int main(int argc, char **argv)
{
	int                   rc=0;
	int                   i;
	pthread_t             threadid[2];

	thread_flag = 1;
	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&mutex, NULL);
  fprintf(stderr, "Initially, thread_flag is %p - %d\n", &thread_flag, thread_flag);
	
	rc = pthread_create(&threadid[0], NULL, thread_function1, NULL);
	rc = pthread_create(&threadid[1], NULL, thread_function2, NULL);

	for (i=0; i<2; ++i) {
		rc = pthread_join(threadid[i], NULL);
		printf("pthread_join() in %d\n", i);
	}
	fprintf(stderr, "pthread_join() finished\n");
	pthread_cond_destroy(&cond);
	fprintf(stderr, "after pthread_join() finished\n");
}
