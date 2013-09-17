/* This test program are adpated from http://www.cs.mtu.edu/~shene/NSF-3/e-Book/MONITOR/Philosopher-5/MON-example-Philos-5.html and operating system concepts book in chapter 6.
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

//#define PHIL_NUM 500
//#define PHIL_NUM 40
#define PHIL_NUM 50

char outbuf[4096];
int outfd = 2;
#define PRINT_SIZE 4096
//#define PRINT(fmt,...) fprintf(stderr, fmt, ##__VA_ARGS__)
#if 0
#define PRINT(fmt,...) \
      { \
      ::snprintf(outbuf, PRINT_SIZE, "%20s:%-4d: " fmt "\n", \
               __FILE__, __LINE__, ##__VA_ARGS__ );  \
      write(outfd, outbuf, strlen(outbuf));  } 
#else
//#define PRINT(fmt,...) fprintf(stderr, fmt, ##__VA_ARGS__)
//#define PRINT(fmt,...) printf(fmt, ##__VA_ARGS__)
#define PRINT(fmt,...) 
#endif
//      ::sprintf(outbuf, "%20s:%-4d: " fmt "\n", \
               __FILE__, __LINE__, ##__VA_ARGS__ );  \
      write(outfd, outbuf, strlen(outbuf));  } 

enum {thinking, hungry, eating} state[PHIL_NUM];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t self[PHIL_NUM];
pthread_mutex_t mutex_exit = PTHREAD_MUTEX_INITIALIZER;
int eat_finished = 0;
pthread_cond_t cond_exit;

void test(int i) {
#define N PHIL_NUM
	if((state[(i-1+N) % N] != eating) &&  // left is not eating 
	   (state[i] == hungry) &&            // I am hungry, and
           (state[(i+1)%N] != eating)) {      // right is not eating
		state[i] = eating;            // This philosopher can eat
	//pthread_mutex_lock(&mutex);
//		PRINT("%d can eat\n", i);
		pthread_cond_signal(&self[i]); // Wake him up. FIXME
	//pthread_mutex_unlock(&mutex);           // Wakenup by others, then I can eat
	}
}

void pickup(int i) {
	pthread_mutex_lock(&mutex);
	state[i] = hungry; 
	test(i);                                // Test if I can eat
	if(state[i] != eating)                  // if the test result says no, then wait
		pthread_cond_wait(&self[i], &mutex);
	pthread_mutex_unlock(&mutex);           // Wakenup by others, then I can eat
}

void putdown(int i) {
	pthread_mutex_lock(&mutex);
	state[i] = thinking;			// go back to thinking
	test((i-1+PHIL_NUM)%PHIL_NUM);
	test((i+1)%PHIL_NUM);
	pthread_mutex_unlock(&mutex);
}


void eat_work(void) {
	int i;
        int f,f1,f2;

        f1 =12441331;
        f2 = 3245235;
        for (i = 47880; i > 0; i--) {
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

void * thread(void * p) {
	int i = (intptr_t)p;
	
	//usleep(random()%1000);
//	usleep(5);
	//fprintf(stderr, "Thread %d (at %p) is in the beginning\n", i, (void*)pthread_self());
	PRINT("Thread %d (at %p) trying to pickup\n", i, (void*)pthread_self());
	pickup(i);
	PRINT("Thread %d (at %p) trying to eat\n", i, (void *)pthread_self());
	eat_work();
	PRINT("After eating, thread %d (at %p) notify its neighbour\n", i, (void *)pthread_self());
	putdown(i);
	
	//Wait on existing conditional variable.
//	pthread_mutex_lock(&mutex_exit);
	pthread_mutex_lock(&mutex);
	eat_finished++;
	//PRINT("Thread %d (at %p) is checking, %d are waiting\n", i, (void *)pthread_self(), eat_finished);
	if(eat_finished == PHIL_NUM) {
//		PRINT("Thread %d waking all phylosophers up\n", i);
		pthread_cond_broadcast(&cond_exit);
	}
	else {
	//	PRINT("Thread %d (at %p) is waiting for others, it has done\n", i, (void *)pthread_self());
		pthread_cond_wait(&cond_exit, &mutex);
	}
	pthread_mutex_unlock(&mutex);
	return((void*)NULL);
}

void init() {
	int i;
	for(i = 0; i < PHIL_NUM; i++) 
		state[i] = thinking;
}

int main() {
	
	int i;
	pthread_t threads[PHIL_NUM];

	srandom(time(NULL));

	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutex_exit, NULL);

	for(i = 0; i < PHIL_NUM; i++) {
		pthread_cond_init(&self[i], NULL);
	}
	pthread_cond_init(&cond_exit, NULL);

	init();
	
	// Create threads
	for(i = 0; i < PHIL_NUM; i++) {
		pthread_create(&threads[i], NULL, thread, (void *)i);
   // fprintf(stderr, "Creating THREAD%d thread %p\n", i, threads[i]);
	}

  //fprintf(stderr, "Create threads %d PHIL_NUM %d\n", i, PHIL_NUM);

	// Join all threads
	for(i = 0; i < PHIL_NUM; i++) {
		pthread_join(threads[i], NULL);
		PRINT("Joining %d thread\n", i);
	}


	// Destroy those conditional variables.
	pthread_cond_destroy(&cond_exit);
	for(i = 0; i < PHIL_NUM; i++) {
		pthread_cond_destroy(&self[i]);
	}
}
