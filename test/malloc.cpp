
#include <iostream>

int temp = 10;

int main()
{
  int * ptr = NULL;

  printf("In the beginning, temp(global) %d at %p!\n", temp, &temp);

  //while(1) ;
  ptr = (int *)malloc(sizeof(int));

  printf("In the beginning, ptr(heap) is at %p!\n", ptr);
  *ptr = 5;
  temp = *ptr; 

  printf("In the middle, temp(global) %d, ptr(heap) is %d at %p!\n", temp, *ptr, ptr);

  free(ptr);

  //ptr = (int *)malloc(16);
  //printf("in the end, ptr at %p\n", ptr);
}
