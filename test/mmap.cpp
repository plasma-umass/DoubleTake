
#include <iostream>
#include <sys/mman.h>

int temp = 10;

int main()
{
  int * ptr = NULL;

  ptr = (int *)mmap (NULL,
             4096,
             PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS,
             -1,
             0);


  *ptr = 5;
  temp = *ptr; 

  printf("In the middle, temp(global) %d, ptr(heap) is %d at %p!\n", temp, *ptr, ptr);

//  munmap(ptr, 4096);

  
  //ptr = (int *)malloc(16);
  //printf("in the end, ptr at %p\n", ptr);
}
