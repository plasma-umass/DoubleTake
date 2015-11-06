#ifndef DOUBLETAKE_DOUBLETAKE_HH
#define DOUBLETAKE_DOUBLETAKE_HH

#include <atomic>

#include <stdint.h>
#include <sys/types.h> // pid_t

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
// ensure that a function call doesn't get optimized away
#define USED(v) ((void)(v))

namespace doubletake {
  extern std::atomic_bool initialized;
  extern std::atomic_bool inRollback;

  struct RegionInfo {
    uintptr_t start;
    uintptr_t end;
  };

  int findStack(pid_t tid, uintptr_t *bottom, uintptr_t *top);

  bool isLib(void *pcaddr);

  void printStackCurrent();
  void printStack(size_t len, void **frames);

  /// global runtime lock - must be held to end an epoch or spawn a
  /// new thread or access the threadmap.  This will also block SIGUSR2 for the thread that
  /// acquired it -- TODO: that may not be necessary, but it
  /// simplifies reasoning for now.
  void lock();
  void unlock();


  /// sets the number of threads we're waiting for - this is
  /// initialized to a positive non-zero number in quiesce() and
  /// decremented once each thread executes its SIGUSR2 handler.
  void setWaiterCount(size_t n);
  void waitUntilQuiescent();
  void currentIsQuiesced();

  void epochComplete();
  void waitForEpochComplete();
}

#endif // DOUBLETAKE_DOUBLETAKE_HH
