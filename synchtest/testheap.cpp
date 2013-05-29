#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

#define TOTAL_COUNT (1000)
#define TOTAL_MALLOCS (TOTAL_COUNT/8) 

void * testheap(void * param) {
	int j = 0;

	for(j = 0; j < TOTAL_MALLOCS; j++) { 
		void * ptr = (int *)malloc(50);
		fprintf(stderr, "50: ptr %p\n", ptr);
		free(ptr);
	}

	for(j = 0; j < TOTAL_MALLOCS; j++) { 
		void * ptr = (int *)malloc(500);
		fprintf(stderr, "500: ptr %p\n", ptr);
		free(ptr);
	}

	for(j = 0; j < TOTAL_MALLOCS; j++) { 
		void * ptr = (int *)malloc(5000);
		fprintf(stderr, "5000: ptr %p\n", ptr);
		free(ptr);
	}


	for(j = 0; j < TOTAL_MALLOCS; j++) { 
		void * ptr = (int *)malloc(50000);
		fprintf(stderr, "50000: ptr %p\n", ptr);
		free(ptr);
	}

	return NULL;
}

int main() {
   pthread_t pid[8];

   pthread_create(&(pid[0]), NULL, testheap, (void *)NULL);
   pthread_join(pid[0], NULL);
}
