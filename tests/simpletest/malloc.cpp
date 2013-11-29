#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv) {
  int i; 
  int * ptr;
  int size = 3;

  for(i = 1; i < 20; i++) {
    size += 1;
    ptr = (int *) malloc(size);
    memset(ptr, 0, size);
    fprintf(stderr, "i %d (malloc size %d): ptr %p\n", i, size, ptr);
    *ptr = 5;
    free(ptr);
  }

  //fprintf(stderr, "malloc with ptr %p value %d\n", ptr, *ptr);
}
