#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int size = 4;
int main(int argc, char ** argv) {
  int i; 
  int * ptr;

  for(i = 1; i < 50; i++) {
    ptr = (int *) malloc(sizeof(int) * i);
    fprintf(stderr, "i %d (malloc size %d): ptr %p\n", i, sizeof(int) * i, ptr);
    *ptr = 5;
    free(ptr);
  }
}
