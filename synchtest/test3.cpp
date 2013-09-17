// Copy from http://www.mcs.csuhayward.edu/~tebo/Classes/6560/threads/sync.html
/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************
 *
 * cvsimple.c
 *
 * Demonstrates pthread cancellation.
 *
 */
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS  3
#define TCOUNT 10
#define COUNT_THRESH 12

int     count = 0;
int     thread_ids[3] = {0,1,2};
pthread_mutex_t count_lock=PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t count_hit_threshold=PTHREAD_COND_INITIALIZER; 

void unit_work(void)
{
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

void *inc_count(void *idp)
{
  int i=0, save_state, save_type;
  int my_id = (intptr_t)idp;

  for (i=0; i<TCOUNT; i++) {
    pthread_mutex_lock(&count_lock);
    count++;
    unit_work();    
    fprintf(stderr, "inc_counter(): thread %d, count = %d, unlocking mutex\n", 
	   my_id, count);
    if (count >= COUNT_THRESH) {
      fprintf(stderr, "inc_count(): Thread %d, count %d\n", my_id, count);
      pthread_cond_signal(&count_hit_threshold);
	  pthread_mutex_unlock(&count_lock);
	  break;
    }
    pthread_mutex_unlock(&count_lock);
  }
 
  fprintf(stderr, "%d: is going to exit\n", getpid()); 
  return(NULL);
}

void *watch_count(void *idp)
{
  int i=0, save_state, save_type;
  int my_id = (intptr_t)idp;

  printf("watch_count(): thread %d\n", my_id);
 
  pthread_mutex_lock(&count_lock);

  while (count < COUNT_THRESH) {
    pthread_cond_wait(&count_hit_threshold, &count_lock);
    fprintf(stderr, "watch_count(): thread %d count %d\n", my_id, count);
  }

  pthread_mutex_unlock(&count_lock);
  
  return(NULL);
}

main()
{
  int       i;
  pthread_t threads[3];

  fprintf(stderr, "Count is %p\n", &count);

  pthread_mutex_init(&count_lock, NULL); 
  pthread_cond_init(&count_hit_threshold, NULL);
  pthread_create(&threads[0], NULL, inc_count, (void *)0);
  pthread_create(&threads[1], NULL, inc_count, (void *)1);
  pthread_create(&threads[2], NULL, watch_count, (void *)2);

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }
}


