#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/* For safe condition variable usage, must use a boolean predicate and  */
/* a mutex with the condition.                                          */
int main(int argc, char **argv)
{
  fprintf(stderr, "In the beginning of main\n");
  void * ptr; 

  fprintf(stderr, "Before Malloc 8\n");
  ptr = malloc(8);
  fprintf(stderr, "After Malloc ptr %p\n", ptr);

  free(ptr);
}
