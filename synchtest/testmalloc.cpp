#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

int * count1, *count2;
#define TOTAL_COUNT (1000)
#define COUNT_PER_THREAD (TOTAL_COUNT/8) 

void * update1(void * param) {
	int j = 0;
	void * ptr = (int *)malloc(50);

	for(j = 0; j < COUNT_PER_THREAD; j++) 
		count1[2]++;
	return NULL;
}

void * update2(void * param) {
	int j = 0;
	void * ptr = (int *)malloc(30);
	for(j = 0; j < COUNT_PER_THREAD; j++) 
		count2[0]++;
	return NULL;
}

int main() {
   pthread_t pid[8];
   int i;

   count1 = (int *)malloc(16);

   pthread_create(&(pid[0]), NULL, update1, (void *)NULL);
   pthread_join(pid[0], NULL);

   free(count1);
	
   printf("count1 is %p\n", count1);

   // allocat object again.
   count2 = (int *) malloc(16);
   pthread_create(&(pid[1]), NULL, update2, (void *)NULL);
   
   pthread_join(pid[1], NULL);

   free(count2);
   printf("count1 is %p\n", count1);
	
}
