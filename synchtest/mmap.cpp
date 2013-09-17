#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>

#define NTHREADS 1

void * threadfunc(void * ptr) {
  int i;
  int j = 0;
  //fprintf(stderr, "child thread******\n");
  while(j < 30) {
    
    for(i = 0; i < 10000000; i++) {
      if(i % 0x100000 == 0) {
    //    fprintf(stderr, "round %d: i is %lx\n", j, i);
      }
    }
    i = 0;
    j++;
  }

  return NULL;
}

int main()
{

  void * ptr = NULL;
  
  int pagesize = 0x1000;
  int rc = 0;  
  int                   i;
  pthread_t             threadid[NTHREADS];
#if 1
  for(i=0; i<NTHREADS; ++i) {
    rc = pthread_create(&threadid[i], NULL, threadfunc, (void *)i);
  }


  sleep(1);
#endif
//  fprintf(stderr, "test1\n");
  ptr = mmap(NULL, pagesize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
  fprintf(stderr, "test1: ptr %p\n", ptr, strerror(errno));
  
  sleep(1);
  fprintf(stderr, "test2\n");
  ptr = mmap(NULL, pagesize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
  fprintf(stderr, "test2: ptr %p\n", ptr);
  
#if 1
  for (i=0; i<NTHREADS; ++i) {
    rc = pthread_join(threadid[i], NULL);
//    printf("pthread_join() in %d\n", i);
  }
#endif
}
