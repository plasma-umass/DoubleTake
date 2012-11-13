
#include <iostream>

#include <stdlib.h>

int main()
{
  int i;

  srandom(100);

  for(i = 0; i < 10; i++) {
    printf("%d -- %d\n", i, random());
  }
}
