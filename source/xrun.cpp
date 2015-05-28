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

void xrun::startRollback() {
  // Now we are going to rollback. Wakup all threads
  // waiting on the shared conditional variable
  global_rollback();

  PRINF("Starting rollback for other threads\n");
  PRINT("Starting rollback for other threads\n");

  // Set context for current thread.
  // Since the new context is not valid, how to
  // rollback?
  xthread::getInstance().rollback();
}

void xrun::syscallsInitialize() { syscalls::getInstance().initialize(); }

void xrun::rollbackandstop() {
  // Now we are going to rollback.
  startRollback();

  PRINF("\n[DoubleTake: Rolling back.]\n");

  // Rollback all memory before rolling back the context.
  _memory.rollbackonly();
}

// We are rollback the child process
void xrun::rollback() {
  // If this is the first time to rollback,
  // then we should rollback now.
  if(global_hasRollbacked()) {
    PRINF("HAS rolled back, now exiting.\n");
    rollbackandstop();
    abort();
  }

  // Rollback all memory before rolling back the context.
  _memory.rollback();

  PRINT("\n\nAFTER MEMORY ROLLBACK!!!\n\n\n");
 
 // We should prepare those system calls after the memory rollback.
  // since memory rollback can destroy filestream in the user space.
  syscalls::getInstance().prepareRollback();
   PRINT("\n\nNOW PREPARE ROLLBACK!!!\n\n\n");

  //  PRINF("_memory rollback!!!\n");
  // We must prepare the rollback, for example, if multiple
  // threads is existing, we must initiate the semaphores for threads
  // Also, we should setup those synchronization event list
  _thread.prepareRollback();
   PRINT("_thread rollback and actual rollback\n");

  // while(1);
  // PRINF("\n\nset rollback\n\n\n");

  // Now we are going to rollback
  PRINF("\n\nSTARTING ROLLBACK!!!\n\n\n");
  startRollback();

  assert(0);
  // FIXME: how to start those temporary stopped threads.
  // Setcontext
}

/// @brief Start a new epoch.
void xrun::epochBegin() {

  threadmap::aliveThreadIterator i;
 
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

	//	PRINF("xrun epochBegin setting the map\n");
    if(thread != current) {
      lock_thread(thread);

      if(thread->hasJoined) {
        thread->status = E_THREAD_EXITING;
        Real::pthread_cond_signal(&thread->cond);
        unlock_thread(thread);
        Real::pthread_join(thread->self, NULL);
      } else {
        unlock_thread(thread);
      }
    }
  }

	// Cleanup the event list.
  xthread::epochBegin();
  
	// Saving the context of the memory.
  _memory.epochBegin();
  xthread::getInstance().runDeferredSyncs();

  // Now waken up all other threads then threads can do its cleanup.
  PRINF("getpid %d: xrun::epochBegin, wakeup others. \n", getpid());
  global_epochBegin();

  // Cleaning up the record of system calls.
  syscalls::getInstance().atEpochBegin();

  PRINF("getpid %d: xrun::epochBegin\n", getpid());

  // Save the context of this thread
  saveContext();
}

/// @brief End a transaction, aborting it if necessary.
void xrun::epochEnd(bool endOfProgram) {
  // Tell other threads to stop and save context.
  stopAllThreads();

  // To avoid endless rollback
  if(global_isRollback()) {
    // PRINF("in the end of an epoch, endOfProgram %d. global_isRollback true\n", endOfProgram);
    while(1)
      ;
  }

#ifdef DEBUG_ROLLBACK
  rollback();
// assert(0);
#endif

  bool hasOverflow = false;
  if (detectOverflow()) {
    hasOverflow = _memory.checkHeapOverflow();
  }

  bool hasMemoryLeak = false;
  if (detectMemoryLeaks()) {
    if(endOfProgram) {
      //  PRINF("DETECTING MEMORY LEAKAGE in the end of program!!!!\n");
      hasMemoryLeak =
        leakcheck::getInstance().doFastLeakCheck(_memory.getHeapBegin(), _memory.getHeapEnd());
    } else {
      // PRINF("DETECTING MEMORY LEAKAGE inside a program!!!!\n");
      hasMemoryLeak =
        leakcheck::getInstance().doSlowLeakCheck(_memory.getHeapBegin(), _memory.getHeapEnd());
    }
  }

#ifndef EVALUATING_PERF
// First, attempt to commit.
#if defined(DETECT_OVERFLOW) && defined(DETECT_MEMORY_LEAKS)
  PRINT("in the end of an epoch, hasOverflow %d hasMemoryLeak %d\n", hasOverflow, hasMemoryLeak);
  if(hasOverflow || hasMemoryLeak) {
    rollback();
  } else {
#elif defined(DETECT_OVERFLOW)
  PRINT("in the end of an epoch, hasOverflow %d\n", hasOverflow);
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
		while(1) {; }
    // PRINF("getpid %d: xrun::epochEnd without overflow\n", getpid());
    syscalls::getInstance().epochEndWell();
    xthread::getInstance().epochEndWell();
#ifndef EVALUATING_PERF
#if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKS)
  }
#endif
#endif
  //PRINF("in the end of an epoch, hasOverflow %d hasMemoryLeak %d\n", hasOverflow, hasMemoryLeak);
}

bool isThreadSafe(thread_t* thread) { return thread->isSafe; }

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

void xrun::stopAllThreads() {
  threadmap::aliveThreadIterator i;
  int waiters = 0;
  int count = 0;
  /* According to description of pthread_kill:
     One comment: The PID field in the TCB can temporarily be changed
     (in fork).  But this must not affect this code here.  Since this
     function would have to be called while the thread is executing
     fork, it would have to happen in a signal handler.  But this is
     no allowed, pthread_kill is not guaranteed to be async-safe.  */
  // threadmap::getInstance().traverseAllThreads();
  global_checkWaiters();

  // Used to tell other threads about normal epoch end because of one have to commit.
  // pthread_kill can not support additional signal value except signal number
  global_setEpochEnd();

  // In order to avoid this problem, we may avoid the thread spawning in this phase.
  global_lock();

  // PRINF("EPOCHEBD:Current thread at %p self %p\n", current, pthread_self());
  PRINF("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^EPOCHEBD:Current thread at %p THREAD%d self %p. "
        "Stopping other threads\n",
        current, current->index, (void*)pthread_self());
  for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    // PRINF("in epochend, thread%d mutex is %p\n", thread->index, &thread->mutex);
    if(thread != current) {
      lock_thread(thread);
      // If the thread's status is already at E_THREAD_WAITFOR_REAPING
      if(thread->status != E_THREAD_WAITFOR_REAPING) {
        if(thread->isSafe) {
          // If the thread is in cond_wait or barrier_wait,
          PRINF("in epochend, stopping thread %p self %p status %d\n", thread, (void*)thread->self,
                thread->status);
          // PRINF("in epochend, thread %p self %p status %d\n", thread, thread->self,
          // thread->status);
          waiters++;
          Real::pthread_kill(thread->self, SIGUSR2);
        } else {
          // PRINF("NOTSAFE!!! Thread %p self %p status %d\n", thread, thread->self,
          // thread->status);
          thread->waitSafe = true;
          while(!thread->isSafe) {
            wait_thread(thread);
          }
          if(thread->status != E_THREAD_WAITFOR_REAPING) {
            Real::pthread_kill(thread->self, SIGUSR2);
            waiters++;
          }
        }
        // Else, if thread is not safe, the thread can wait by itself.
      }

      unlock_thread(thread);
    }
    count++;
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

// Handling the signal SIGUSR2
/* We are using the SIGUSR2 to notify other threads something.

 */
 void xrun::sigusr2Handler(int /* signum */, siginfo_t* /* siginfo */, void* context) {
  // Check what is current status of the system.
  // If we are in the end of an epoch, then we save the context somewhere since
  // current thread is going to stop execution in order to commit or rollback.
  assert(global_isEpochEnd() == true);

  // printf("Thread %p in sigusr2Handler %d\n", pthread_self(), __LINE__);
  // Wait for notification from the commiter
  global_waitForNotification();

  // Check what is the current phase: rollback or resume
  if(global_isEpochBegin()) {
    PRINF("%p wakeup from notification.\n", (void*)pthread_self());
    // PRINF("%p reset contexts~~~~~\n", pthread_self());
    xthread::epochBegin();
    xthread::getInstance().saveSpecifiedContext((ucontext_t*)context);
    syscalls::getInstance().atEpochBegin();
    // xthread::getInstance().resetContexts();
    // NOTE: we do not need to reset contexts if we are still inside the signal handleer
    // since the exiting from signal handler can do this automatically.
  } else {
    PRINF("epochBegin %d rollback %d\n", global_isEpochBegin(), global_isRollback());
    assert(global_isRollback() == true);

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
  // jumpToFunction((ucontext_t *)context, (unsigned long)afterSignalHandler);
}
