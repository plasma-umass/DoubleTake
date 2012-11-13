
#include <iostream>

#include <stdlib.h>

int main()
{
  int i;

  //while(1) ; 
  for(i = 0; i < 4; i++) {
    printf("%d -- %d\n", i, random());
  }
}
