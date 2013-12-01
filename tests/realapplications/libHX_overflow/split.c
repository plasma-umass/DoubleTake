#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
//#include "string.h"

char **HX_split(const char *str, const char *delim,
    int *cp, int max);

int main(void)
{
  fprintf(stderr, "in the  beginning of main\n");
  char * src = "It is a test for buffer overflows in HX_Split";  
  char * delim = "is";
  int count = 2;

  HX_split(delim, src, NULL, 5);   

  fprintf(stderr, "in the end of main\n");
  return 0;
}
