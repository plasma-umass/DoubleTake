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
  ptr = (int *) malloc(sizeof(int));
  *ptr = 5;
  //fprintf(stderr, "temp malloc with ptr %p\n", ptr);
  ptr = NULL;
}

int main(int argc, char ** argv) {
  testmalloc();
}
