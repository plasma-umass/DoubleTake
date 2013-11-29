#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char ** argv) {
  int * ptr;

  ptr = (int *) malloc(sizeof(int));

  ptr++;

  *ptr = 5;

  fprintf(stderr, "malloc with ptr %p value %d\n", ptr, *ptr);
}
