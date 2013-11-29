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
  fprintf(stderr, "malloc with ptr %p value %d\n", ptr, *ptr);

//  ptr = NULL;
}

int main(int argc, char ** argv) {
  int fd;
  testmalloc();

  struct stat status;  
  fd = open("./test", O_RDWR);

  fprintf(stderr, "BEFORE fstat........\n");
  fstat(fd, &status);
  fprintf(stderr, "AFTER fstat........\n");
}
