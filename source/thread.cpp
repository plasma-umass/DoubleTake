
#include "threadstruct.hh"
#include "real.hh"

void DT::Thread::initialize(int idx) {
  this->index = idx;

  this->available = false;
  this->internalheap = false;
  this->isNewlySpawned = true;
  this->parent = nullptr;
  this->joiner = nullptr;
  this->hasJoined = false;
  this->isSafe = false;
  this->isDetached = false;
  this->condwait = nullptr;
  this->mainThread = false;

  this->startRoutine = nullptr;
  this->startArg = nullptr;
  this->result = nullptr;

  this->syscalls.initialize(xdefines::MAX_SYSCALL_ENTRIES);

  // Initilize the list of system calls.
  for(size_t i = 0; i < E_SYS_MAX; i++) {
    listInit(&this->syslist[i]);
  }

  // Initialize this syncevents.
  this->syncevents.initialize(xdefines::MAX_SYNCEVENT_ENTRIES);

  // Starting
  Real::pthread_mutex_init(&_mutex, NULL);
  Real::pthread_cond_init(&_cond, NULL);
}

void DT::Thread::lock() { Real::pthread_mutex_lock(&_mutex); }
void DT::Thread::unlock() { Real::pthread_mutex_unlock(&_mutex); }
void DT::Thread::wait() { Real::pthread_cond_wait(&_cond, &_mutex); }
void DT::Thread::signal() { Real::pthread_cond_broadcast(&_cond); }
