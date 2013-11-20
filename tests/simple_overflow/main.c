#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  int* p = (int*)malloc(sizeof(char));
  *p = 12345;
  printf("Hello overflow\n");
  return 0;
}
