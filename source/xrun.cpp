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
#include "internalsyncs.hh"
#include "leakcheck.hh"
#include "syscalls.hh"
#include "threadmap.hh"
#include "threadstruct.hh"


void xrun::syscallsInitialize() { syscalls::getInstance().initialize(); }

void xrun::rollback() {
  // If this is the first time to rollback,
  // then we should rollback now.
  if(global_hasRollbacked()) {
    PRINF("HAS rolled back, now exiting.\n");
    abort();
  }

  // Rollback all memory before rolling back the context.
  _memory.rollback();

  PRINF("\n\nAFTER MEMORY ROLLBACK!!!\n\n\n");
 
  // We must prepare the rollback, for example, if multiple
  // threads is existing, we must initiate the semaphores for threads
  // Also, we should setup those synchronization event list
  _thread.prepareRollback();
   PRINF("_thread rollback and actual rollback\n");

  // Now we are going to rollback
  PRINF("\n\nSTARTING ROLLBACK!!!\n\n\n");

	// Now time to rollback myself.
  _thread.checkRollbackCurrent();
 
	assert(0);
}

/// @brief Start a new epoch.
void xrun::epochBegin() {

  threadmap::aliveThreadIterator i;
 
  PRINF("xrun epochBegin, joinning every thread.\n");
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    lock_thread(thread);

    if(thread != current && thread->hasJoined) {
      PRINF("xrun, joining thread %d\n", thread->index);
      thread->status = E_THREAD_EXITING;
      Real::pthread_cond_signal(&thread->cond);
      unlock_thread(thread);
      PRINF("xrun, actually joining thread %d\n", thread->index);
      Real::pthread_join(thread->self, NULL);
      PRINF("xrun, after joining thread %d\n", thread->index);
      continue;
    }
    // Since now we are in a new epoch mark all existing threads as old threads
    thread->isNewlySpawned = false;

    // cleanup the threads's qlist, pendingSyncevents, syncevents
    xthread::epochBegin(thread);

    unlock_thread(thread);
  }
  PRINF("xrun epochBegin, joinning every thread done.\n");

  xthread::runDeferredSyncs();

  PRINF("xrun epochBegin, run deferred synchronizations done.\n");

  // Now waken up all other threads then threads can do its cleanup.
  PRINF("getpid %d: xrun::epochBegin, wakeup others. \n", getpid());
  global_epochBegin();

  PRINF("getpid %d: xrun::epochBegin\n", getpid());
	
	// Saving the context of the memory.
  _memory.epochBegin();

  // Save the context of this thread
  saveContext();
}

/// @brief End a transaction, aborting it if necessary.
void xrun::epochEnd(bool endOfProgram) {

	fprintf(stderr, "xrun epochEnd\n");
	//while(1) { ; }
//	selfmap::getInstance().printCallStack();
  // Tell other threads to stop and save context.
  stopAllThreads();

  // To avoid endless rollback
  if(global_isRollback()) {
    // PRINF("in the end of an epoch, endOfProgram %d. global_isRollback true\n", endOfProgram);
    while(1)
      ;
  }

#if defined(DETECT_OVERFLOW)
  bool hasOverflow = false;
  hasOverflow = _memory.checkHeapOverflow();
#endif

#if defined(DETECT_MEMORY_LEAKS)
  bool hasMemoryLeak = false;
  if(endOfProgram) {
    //  PRINF("DETECTING MEMORY LEAKAGE in the end of program!!!!\n");
    hasMemoryLeak =
        leakcheck::getInstance().doFastLeakCheck(_memory.getHeapBegin(), _memory.getHeapEnd());
  } else {
    // PRINF("DETECTING MEMORY LEAKAGE inside a program!!!!\n");
    hasMemoryLeak =
      leakcheck::getInstance().doSlowLeakCheck(_memory.getHeapBegin(), _memory.getHeapEnd());
  }
#endif

#ifndef EVALUATING_PERF
// First, attempt to commit.
#if defined(DETECT_OVERFLOW) && defined(DETECT_MEMORY_LEAKS)
  PRINF("in the end of an epoch, hasOverflow %d hasMemoryLeak %d\n", hasOverflow, hasMemoryLeak);
  if(hasOverflow || hasMemoryLeak) {
    rollback();
  } else {
#elif defined(DETECT_OVERFLOW)
  PRINF("in the end of an epoch, hasOverflow %d\n", hasOverflow);
  if(hasOverflow) {
    rollback();
  } else {
#elif defined(DETECT_MEMORY_LEAKS)
  if(hasMemoryLeak) {
    // EDB FIX ME DISABLED
    // _memory.cleanupFreeList();
    rollback();
  } else {
#endif
#endif
    
		PRINF("before calling syscalls epochEndWell\n");
    syscalls::getInstance().epochEndWell();

		xthread::getInstance().epochEndWell();

#ifndef EVALUATING_PERF
#if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKS)
  }
#endif
#endif
  //PRINF("in the end of an epoch, hasOverflow %d hasMemoryLeak %d\n", hasOverflow, hasMemoryLeak);
}

#ifdef DETECT_USAGE_AFTER_FREE
void xrun::finalUAFCheck() {
  threadmap::aliveThreadIterator i;
  // Check all threads' quarantine list
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    if(thread->qlist.finalUAFCheck()) {
      rollback();
    }
  }
}
#endif

void waitThreadSafe(void) {
	int i = 0; 
	while(i++ < 0x10000) ; 
}

void xrun::stopAllThreads() {
  threadmap::aliveThreadIterator i;
  int waiters = 0;
  /* According to description of pthread_kill:
     One comment: The PID field in the TCB can temporarily be changed
     (in fork).  But this must not affect this code here.  Since this
     function would have to be called while the thread is executing
     fork, it would have to happen in a signal handler.  But this is
     no allowed, pthread_kill is not guaranteed to be async-safe.  */
  global_checkWaiters();

  // Used to tell other threads about the end of current epoch end since one have to commit.
  global_setEpochEnd();

  // Grab the global lock in order to avoid the thread spawning in this phase.
  global_lock();

  // PRINF("EPOCHEBD:Current thread at %p self %p\n", current, pthread_self());
  PRINF("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^EPOCHEBD:Current thread at %p THREAD%d self %p. "
        "Stopping other threads\n",
        current, current->index, (void*)pthread_self());

	// Traverse the thread map to check the status of every thread.
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

		// we only care about other threads
    if(thread != current) {
		  lock_thread(thread);
      	
			while(!xthread::isThreadSafe(thread)) {
				// wait the thread to be safe.
        waitThreadSafe();
      }
      // If the thread's status is already at E_THREAD_WAITFOR_REAPING
			// or E_THREAD_JOINING, thus waiting on internal lock, do nothing since they have stopped.
     if((thread->status != E_THREAD_WAITFOR_REAPING) && (thread->status != E_THREAD_JOINING) && (thread->status != E_THREAD_COND_WAITING)) {
        waiters++;
				PRINF("kill thread %d\n", thread->index);
        Real::pthread_kill(thread->self, SIGUSR2);
			}
      unlock_thread(thread);
    }
  }

  if(waiters != 0) {
    global_waitThreadsStops(waiters);
  }

  global_unlock();
}

bool isNewThread() { return current->isNewlySpawned; }

void jumpToFunction(ucontext_t* cxt, unsigned long funcaddr) {
  PRINF("%p: inside signal handler %p.\n", (void*)pthread_self(),
        (void*)cxt->uc_mcontext.gregs[REG_RIP]);
  // selfmap::getInstance().printCallStack(NULL, NULL, true);
  cxt->uc_mcontext.gregs[REG_RIP] = funcaddr;
}

/* 
  We are using the SIGUSR2 to stop other threads.
 */
 void xrun::sigusr2Handler(int /* signum */, siginfo_t* /* siginfo */, void* context) {
  // Check what is current status of the system.
  // If we are in the end of an epoch, then we save the context somewhere since
  // current thread is going to stop execution in order to commit or rollback.
  assert(global_isEpochEnd() == true);

  // Wait for notification from the commiter
  global_waitForNotification();

  // Check what is the current phase
  if(global_isEpochBegin()) {
 		// Current thread is going to enter a new phase
    xthread::getInstance().saveSpecifiedContext((ucontext_t*)context);
    // NOTE: we do not need to reset contexts if we are still inside the signal handleer
    // since the exiting from signal handler can do this automatically.
  } else {
    PRINF("epochBegin %d rollback %d\n", global_isEpochBegin(), global_isRollback());
    assert(global_isRollback() == true);
		
		// Check where we should park, on my own cond or common cond 
    if(isNewThread()) {
      lock_thread(current);

      // Waiting for the waking up from the its parent thread
      while(current->status != E_THREAD_ROLLBACK) {
        Real::pthread_cond_wait(&current->cond, &current->mutex);
      }

      unlock_thread(current);
    }
    // Rollback inside signal handler is different
    xthread::getInstance().rollbackInsideSignalHandler((ucontext_t*)context);
  }
  // Jump to a function and wait for the instruction of the committer thread.
}
