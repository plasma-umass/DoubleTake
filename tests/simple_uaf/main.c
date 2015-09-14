#include <stdio.h>
#include <stdlib.h>

#define USED(v) ((void)(v))

int main(int argc, char** argv) {
  int* p = (int*)malloc(sizeof(int));
  *p = 123;
  printf("Hello use after free\n");
  free(p);
  for (int i = 0; i < 1000; i++) {
    volatile int q = 1.0 * 1.0;
    USED(q);
  }
  *p = 456;
  return 0;
}
