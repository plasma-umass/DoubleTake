/*
 * @file   xrun.cpp
 * @brief  The main engine for consistency management, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xrun.hh"

#include <assert.h>
#include <pthread.h>
#include <ucontext.h>

#include "globalinfo.hh"
#include "leakcheck.hh"
#include "syscalls.hh"
#include "threadmap.hh"
#include "threadstruct.hh"

#ifndef DETECT_MEMORY_LEAKS
#define DETECT_MEMORY_LEAKS 0
#endif

#ifndef DETECT_OVERFLOW
#define DETECT_OVERFLOW 0
#endif

#ifndef DETECT_USAGE_AFTER_FREE
#define DETECT_USAGE_AFTER_FREE 0
#endif

xrun::xrun()
  : _epochId(0), _syscalls(), _memory(), _thread(),
    _watchpoint(watchpoint::getInstance()), _leakcheck()
{
  // we need early-initialization of our syscall wrappers to ensure
  // that libraries like glib can do whatever syscalls they need.
  Real::initializer();

  _thread.getThreadinfo()->initialize();

  // PRINT("xrun: initialization at line %d\n", __LINE__);

  // Initialize the internal heap at first.
  InternalHeap::getInstance().initialize();

  _memory.initialize();

  // Initialize the locks and condvar used in epoch switches
  global_initialize();

  _thread.initialize();

  // and some of those library functions, like the sysrecord for
  // gettimeofday alloc, and that currently accesses current, so
  // ensure that is setup too.
  _thread.registerInitialThread(&_memory);
  current->isSafe = true;

  _syscalls.initialize();

  doubletake::initialized = true;

  // epochBegin pairs with the global lock taken in epochEnd, so we
  // need to grab that lock before we begin our first epoch
  doubletake::lock();
  epochBegin();
}

void xrun::finalize() {
#ifdef GET_CHARECTERISTICS
  fprintf(stderr, "DOUBLETAKE performed %ld epochs\n", _epochId);
#endif
  // If we are not in rollback phase, then we should check buffer overflow.
  if(!doubletake::inRollback) {
    epochEnd(true);
  }

  //    PRINF("%d: finalize now !!!!!\n", getpid());
  // Now we have to cleanup all semaphores.
  _thread.finalize();
}

char *getCurrentThreadBuffer() {
  return current->outputBuf;
}

void xrun::rollback() {
  PRINT("DoubleTake: Activating rollback to isolate error.");
  doubletake::inRollback = true;

  // first rollback all memory, then rollback all thread state
  _memory.rollback();
  _thread.rollback();

  // when we rollback ourselves, we will end up calling epochEnd()
  _thread.rollbackCurrent();

  assert(0);
}

/// @brief Start a new epoch.
void xrun::epochBegin() {

  threadmap::aliveThreadIterator i;
 
  PRINF("xrun epochBegin, joinning every thread.");
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    DT::Thread *thread = i.getThread();

    if(thread != current && thread->hasJoined) {
      PRINF("xrun, joining thread %d\n", thread->index);
      thread->status = E_THREAD_EXITING;
      thread->signal();
      PRINF("xrun, actually joining thread %d\n", thread->index);
      Real::pthread_join(thread->self, NULL);
      PRINF("xrun, after joining thread %d\n", thread->index);
      continue;
    }

    // Since now we are in a new epoch mark all existing threads as
    // old threads
    thread->isNewlySpawned = false;

    // cleanup the threads's qlist, pendingSyncevents, syncevents
    _thread.epochBegin(thread);
  }

  PRINF("xrun epochBegin, joinning every thread done.\n");

  _thread.runDeferredSyncs();
	
	// snapshot the heap and globals - we need to do this while other
	// threads are asleep.
  _memory.epochBegin();

  // Save the context of this thread
  current->context.saveCurrent();

  doubletake::epochComplete();
  doubletake::unlock();
}

/// @brief End a transaction, aborting it if necessary.
void xrun::epochEnd(bool endOfProgram) {

  // this is paired with an unlock in xrun::epochEnd
  doubletake::lock();

  // count how many epochs we've had for performance analysis
  _epochId++;

  // get all other threads to halt -- if we are single-threaded this
  // is a NOP.
  quiesce();

  // TODO: check if this epoch was a rollback epoch, and either exit
  // or continue w/o instrumentation.  The idea is that if we've
  // gotten to the end of an epoch without triggering an exit, the
  // initial problem must have been a race, and when we replayed the
  // epoch we didn't hit the race this time.

  bool hasOverflow = false;
  bool hasMemoryLeak = false;
  bool hasUAF = false;
  
  if (DETECT_OVERFLOW)
    hasOverflow = _memory.checkHeapOverflow();

  if (DETECT_MEMORY_LEAKS) {
    if(endOfProgram) {
      //  PRINF("DETECTING MEMORY LEAKAGE in the end of program!!!!\n");
      hasMemoryLeak = _leakcheck.doFastLeakCheck(_memory);
    } else {
      // PRINF("DETECTING MEMORY LEAKAGE inside a program!!!!\n");
      hasMemoryLeak = _leakcheck.doSlowLeakCheck(_memory);
    }
  }

  if (endOfProgram && DETECT_USAGE_AFTER_FREE)
    hasUAF = finalUAFCheck();

  PRWRN("DoubleTake: At the end of an epoch, hasOverflow %d hasMemoryLeak %d hasUAF %d\n",
        hasOverflow, hasMemoryLeak, hasUAF);

  if (hasOverflow || hasMemoryLeak || hasUAF) {
    rollback();
  } else {
		PRINF("before calling syscalls epochEndWell\n");
    _syscalls.epochEndWell();
		_thread.epochEndWell();
  }
}

void xrun::quiesce() {
  threadmap& tm = threadmap::getInstance();
  threadmap::aliveThreadIterator i;
  int waiters = 0;

  // Traverse the thread map to check the status of every thread
  // except ourselves.  We need to do this twice -- first to get the
  // number of other threads, so that we can initialize the barrier.
  // And then again to actually send the signal to all the other
  // threads.

  for(i = tm.begin(); i != tm.end(); i++) {
    DT::Thread *thread = i.getThread();
    if (thread == current)
      continue;
    waiters++;
  }

  // bonus: if there are no other threads, just exit now
  if (!waiters)
    return;

  doubletake::setWaiterCount(waiters);

  for(i = tm.begin(); i != tm.end(); i++) {
    DT::Thread *thread = i.getThread();
    if (thread == current)
      continue;

    PRINF("sending SIGUSR2 to thread %d", thread->index);
    Real::pthread_kill(thread->self, SIGUSR2);
  }

  doubletake::waitUntilQuiescent();
}

bool xrun::finalUAFCheck() {
  threadmap::aliveThreadIterator i;
  // Check all threads' quarantine list
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    DT::Thread *thread = i.getThread();
    if(thread->qlist.finalUAFCheck())
      return true;
  }
  return false;
}

bool isNewThread() { return current->isNewlySpawned; }

static void jumpToFunction(ucontext_t* cxt, unsigned long funcaddr) {
//  PRINF("%p: inside signal handler %p.\n", (void*)pthread_self(),
//        (void*)cxt->uc_mcontext.gregs[REG_IP]);
  // selfmap::getInstance().printCallStack(NULL, NULL, true);
  cxt->uc_mcontext.gregs[REG_IP] = funcaddr;
}

static void rollbackOutsideHandler() {
  current->context.rollback();
}

void xrun::rollbackFromSegv()
{
  xrun::getInstance().rollbackFromSegvSignal();
}

void xrun::rollbackFromSegvSignal() {
  PRWRN("rolling back due to SEGV");

  // Check whether the segmentation fault is called by buffer overflow.
  if(_memory.checkHeapOverflow()) {
    PRINF("OVERFLOW causes segmentation fault!!!! ROLLING BACK\n");
  }

	rollback();
}

/*
  We are using the SIGUSR2 to stop other threads.
*/
void xrun::sigusr2Handler(int /* signum */, siginfo_t* /* siginfo */, void* uctx) {
  xrun::getInstance().endOfEpochSignal((ucontext_t *)uctx);
}

void xrun::endOfEpochSignal(ucontext_t *uctx) {

  const int epochID = doubletake::currentIsQuiesced();
  doubletake::waitForEpochComplete(epochID);

  if (!doubletake::inRollback) {
    _thread.saveContext((ucontext_t*)uctx);
    return;
  }

  // Rollback inside signal handler is different
  //current->context.rollbackInHandler(uctx);
  jumpToFunction(uctx, (unsigned long)rollbackOutsideHandler);
}

void xrun::sigsegvHandler(int /* signum */, siginfo_t* siginfo, void* uctx) {
  //jumpToFunction((ucontext_t*)uctx, (unsigned long)rollbackFromSegv);
}

void xrun::installSignalHandlers() {
  // Set up an alternate signal stack.
  memset(&current->altstack, 0, sizeof(current->altstack));
  current->altstack.ss_sp = MM::mmapAllocatePrivate(SIGSTKSZ);
  current->altstack.ss_size = SIGSTKSZ;
  current->altstack.ss_flags = 0;
  Real::sigaltstack(&current->altstack, (stack_t *)NULL);

  /**
     Some parameters used here:
     SA_RESTART: Provide behaviour compatible with BSD signal
                 semantics by making certain system calls restartable across signals.
     SA_SIGINFO: The  signal handler takes 3 arguments, not one.  In this case, sa_sigac-
                 tion should be set instead of sa_handler.
     So, we can acquire the user context inside the signal handler

     We do NOT want SA_RESTART - that will cause the signal to
     potentially be recursively received.
  */

  struct sigaction sigusr2;
  memset(&sigusr2, 0, sizeof(sigusr2));
  // no need to explicitly set SIGUSR2 in the mask - it is
  // automatically blocked while the handler is running.
  sigemptyset(&sigusr2.sa_mask);
  sigusr2.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
  sigusr2.sa_sigaction = xrun::sigusr2Handler;
  if(Real::sigaction(SIGUSR2, &sigusr2, NULL) == -1) {
    FATAL("sigaction(SIGUSR2): %d (%s)", errno, strerror(errno));
  }

  struct sigaction sigsegv;
  memset(&sigsegv, 0, sizeof(sigsegv));
  // no need to explicitly set SIGSEGV in the mask - it is
  // automatically blocked while the handler is running.
  sigemptyset(&sigsegv.sa_mask);
  sigsegv.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
  sigsegv.sa_sigaction = xrun::sigsegvHandler;
  //if(Real::sigaction(SIGSEGV, &sigsegv, NULL) == -1) {
  //  FATAL("sigaction(SIGSEGV): %d (%s)", errno, strerror(errno));
  //}
}
