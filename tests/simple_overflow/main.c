#include <stdio.h>
#include <stdlib.h>

extern void overflow (void * buf, int actualSize, int overflowSize);

int main(int argc, char** argv) {
	char buff[256];
	fprintf(stderr, "In the beginning of main function\n");
  int* p = (int*)malloc(sizeof(char));
	fprintf(stderr, "after malloc, p is %p\n", p);
  overflow (p, sizeof(char), 4);
  //  *p = 12345;
//  free(p);
	fprintf(stderr, "Right before the return of main function\n");
	
  return 0;
}
