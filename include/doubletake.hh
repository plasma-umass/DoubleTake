#ifndef DOUBLETAKE_DOUBLETAKE_HH
#define DOUBLETAKE_DOUBLETAKE_HH

#include <stdint.h>
#include <sys/types.h> // pid_t

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

namespace doubletake {

  struct RegionInfo {
    uintptr_t start;
    uintptr_t end;
  };

  int findStack(pid_t tid, uintptr_t *bottom, uintptr_t *top);

  bool isLib(void *pcaddr);

  void printStackCurrent();
  void printStack(size_t len, void **frames);
}

#endif // DOUBLETAKE_DOUBLETAKE_HH
