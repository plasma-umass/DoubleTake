#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char ** argv) {
  int * ptr;
  int fd;
  struct stat status;  
  ptr = (int *) malloc(sizeof(int));

  *ptr = 5;

  fd = open("./test", O_RDWR);

  fprintf(stderr, "malloc with ptr %p value %d fd %d\n", ptr, *ptr, fd);
  //fprintf(stderr, "BEFORE fstat........\n");
  //fstat(fd, &status);
  //fprintf(stderr, "AFTER fstat........\n");
}
