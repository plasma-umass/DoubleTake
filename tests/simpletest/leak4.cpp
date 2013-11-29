#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int * ptr = NULL;
// An actual memory leakage
int testmalloc(void) {
  int * ptr2; 
  ptr = (int *) malloc(sizeof(int));
  fprintf(stderr, "ptr is %p\n", ptr);
  ptr2 = (int *)malloc(sizeof(int));
  *ptr = 5;
  fprintf(stderr, "malloc with ptr %p value %d\n", ptr, *ptr);
  free(ptr);
  ptr = NULL;
}

int main(int argc, char ** argv) {
  testmalloc();
}
