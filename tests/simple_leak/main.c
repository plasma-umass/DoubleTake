#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  int* p = (int*)malloc(sizeof(int));
  p = NULL;
  printf("Hello leak\n");
  return 0;
}
