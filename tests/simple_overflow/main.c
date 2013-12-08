#include <stdio.h>
#include <stdlib.h>

extern void overflow (void * buf, int actualSize, int overflowSize);

int main(int argc, char** argv) {
  int* p = (int*)malloc(sizeof(char));
  overflow (p, sizeof(char), 4);
  //  *p = 12345;
  printf("Hello overflow\n");
  free(p);
  return 0;
}
