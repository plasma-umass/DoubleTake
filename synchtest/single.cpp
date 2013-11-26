#include<stdio.h>
#include<stdlib.h>
#include <iostream>

int temp = 10;

int main()
{
  int * ptr = NULL;

  printf("In the beginning, temp(global) %d, ptr(heap) is  %p!\n", temp, ptr);
  ptr = (int *)malloc(sizeof(int));

  *ptr = 5;
  temp = *ptr; 

  printf("In the end of main, temp(global) %d, ptr(heap) is %d at %p!\n", temp, *ptr, ptr);
}
