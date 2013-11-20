#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  int* p = (int*)malloc(sizeof(int));
  *p = 123;
  printf("Hello use after free\n");
  free(p);
  *p = 456;
  return 0;
}
