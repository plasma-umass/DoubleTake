#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char ** argv) {
  int * ptr;
  ptr = (int *) malloc(sizeof(int));

  *ptr = 5;
}
