#include <stdlib.h>
#include <stdio.h>
#include <iostream>

int temp = 10;

int main()
{
  int * ptr = NULL;

  printf("In the beginning, temp(global) %d, ptr(heap) is  %p!\n", temp, ptr);
  ptr = (int *)malloc(sizeof(int));

  *ptr = 5;
  temp = *ptr; 

  printf("In the middle, temp(global) %d, ptr(heap) is %d at %p!\n", temp, *ptr, ptr);

  free(ptr);

  ptr = (int *)malloc(sizeof(int));

  *ptr = 5;
  
  printf("In the middle, malloc the same size after free,  ptr(heap) is %d at %p!\n",  *ptr, ptr);

  ptr = (int *)malloc(16);
  printf("in the end, ptr at %p\n", ptr);
}
