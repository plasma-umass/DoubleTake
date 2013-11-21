#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  int* p = (int*)malloc(sizeof(int));
  printf("Hello leak\n");
  p = NULL;
  return 0;
}
