#include <stdio.h>
#include <stdlib.h>

enum {
  InitialMallocSize = 512 * 1024 * 1024
};

// Temporary bump-pointer allocator for malloc() calls before DoubleTake is initialized
void * tempmalloc(int size) {
  static char _buf[InitialMallocSize];
  static int _allocated;
  
  if(_allocated + size > InitialMallocSize) {
   	printf("Not enough space for tempmalloc");
    abort();
  } else {
    void* p = (void*)&_buf[_allocated];
    _allocated += size;
    return p;
  }
}
