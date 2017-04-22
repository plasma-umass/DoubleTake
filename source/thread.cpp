
#include <sys/types.h>

#include "threadstruct.hh"
#include "doubletake.hh"
#include "real.hh"
#include "xmemory.hh"
#include "xrun.hh"

static pid_t gettid() { return syscall(SYS_gettid); }

void DT::Thread::allocate(int idx) {
  this->index = idx;

  this->_useInternalHeap = false;
  this->_enableChecks = false;

  this->parent = current;

  this->creationEpoch = doubletake::epochID();
  this->status = E_THREAD_STARTING;
  this->isNewlySpawned = true;
  this->available = false;
  this->joiner = nullptr;
  this->hasJoined = false;
  this->isSafe = false;
  this->isDetached = false;
  this->condwait = nullptr;
  this->isMain = false;

  this->startRoutine = nullptr;
  this->startArg = nullptr;
  this->result = nullptr;

  this->tid = -1;
  this->self = 0;

  this->syscalls.initialize(xdefines::MAX_SYSCALL_ENTRIES);

  // Initilize the list of system calls.
  for(size_t i = 0; i < E_SYS_MAX; i++) {
    listInit(&this->syslist[i]);
  }

  //listInit(&this->list);

  // Initialize this syncevents.
  this->syncevents.initialize(xdefines::MAX_SYNCEVENT_ENTRIES);

  // Starting
  Real::pthread_mutex_init(&_mutex, NULL);
  Real::pthread_cond_init(&_cond, NULL);
}

void DT::Thread::initialize(bool isMain, xmemory* memory) {

  size_t stackSize = __max_stack_size;

  this->self = Real::pthread_self();
  this->isMain = isMain;
  this->status = E_THREAD_RUNNING;
  this->tid = gettid();

  listInit(&this->pendingSyncevents);

  uintptr_t privateTop = 0;
  if (isMain) {
    uintptr_t stackBottom;
    // First, we must get the stack corresponding information.
    memory->findStack(gettid(), &stackBottom, &privateTop);
  } else {
    /*
      Currently, the memory layout of a thread private area is like the following.
      ----------------------  Higher address
      |      TCB           |
      ---------------------- pd (pthread_self)
      |      TLS           |
      ----------------------
      |      Stacktop      |
      ---------------------- Lower address
    */
    // Calculate the top of this page.
    privateTop = ((uintptr_t)this->self + xdefines::PageSize) & ~xdefines::PAGE_SIZE_MASK;
  }

  this->context.setupStackInfo((void *)privateTop, stackSize);
  this->stackTop = (void *)privateTop;
  this->stackBottom = (void*)((intptr_t)privateTop - stackSize);

  if (!this->altstack.ss_sp) {
    this->altstack.ss_sp = MM::mmapAllocatePrivate(SIGSTKSZ);
    this->altstack.ss_size = SIGSTKSZ;
    this->altstack.ss_flags = 0;
  }
}

void DT::Thread::makeSafe() {
  _useInternalHeap = false;
  _enableChecks = true;
}
void DT::Thread::makeUnsafe() {
  _useInternalHeap = true;
  _enableChecks = false;
}

void DT::Thread::lock() { Real::pthread_mutex_lock(&_mutex); }
void DT::Thread::unlock() { Real::pthread_mutex_unlock(&_mutex); }
void DT::Thread::wait() { Real::pthread_cond_wait(&_cond, &_mutex); }
void DT::Thread::signal() { Real::pthread_cond_broadcast(&_cond); }