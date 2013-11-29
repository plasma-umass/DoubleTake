#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// An actual memory leakage
int testmalloc(void) {
  int * ptr;
  ptr = (int *) malloc(sizeof(int));

  *ptr = 5;
  fprintf(stderr, "malloc with ptr %p value %d\n", ptr, *ptr);
}

int main(int argc, char ** argv) {
  testmalloc();
}
