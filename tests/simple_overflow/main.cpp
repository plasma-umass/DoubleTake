#include <stdio.h>
#include <stdlib.h>

extern void overflow (void * buf, int actualSize, int overflowSize);

int main(int argc, char** argv) {
  int * p = new int[1];
  overflow (p, sizeof(int), 1);

  return 0;
}
