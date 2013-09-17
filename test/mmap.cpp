
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

  printf("In the beginning, mmap at  %p!\n", ptr);
  
  ptr = (int *)mmap (NULL,
             4096,
             PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS,
             -1,
             0);

  printf("In the end, mmap at  %p!\n", ptr);

  while(1);

//  munmap(ptr, 4096);

  
  //ptr = (int *)malloc(16);
  //printf("in the end, ptr at %p\n", ptr);
}
