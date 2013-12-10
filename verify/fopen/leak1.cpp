#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char ** argv) {
  int * ptr;
	FILE * fp;
  struct stat status;  
  ptr = (int *) malloc(sizeof(int));

  *ptr = 5;

  fp = fopen("./test", "r+");

  fprintf(stderr, "malloc with ptr %p value %d fp %p\n", ptr, *ptr, fp);
  //fprintf(stderr, "BEFORE fstat........\n");
  //fstat(fd, &status);
  fprintf(stderr, "AFTER fstat........\n");
}
