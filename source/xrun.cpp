// -*- C++ -*-

/*
  Copyright (c) 2012, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   xrun.cpp
 * @brief  The main engine for consistency management, etc.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xrun.h"
#include "syscalls.h"
#include "globalinfo.h"
#include "threadmap.h"
#include "leakcheck.h"

void xrun::startRollback(void) {
  // Now we are going to rollback. Wakup all threads
  // waiting on the shared conditional variable
  global_rollback();

  WARN("Starting rollback for other threads\n");

  // Set context for current thread.
  // Since the new context is not valid, how to 
  // rollback?
  xthread::getInstance().rollback();
}

void xrun::syscallsInitialize(void) {
   syscalls::getInstance().initialize();
}

void xrun::rollbackandstop(void) {
    // Now we are going to rollback.
    startRollback();

    DEBUG("\n\nNOW ROLLING BACK\n\n\n");
  
    // Rollback all memory before rolling back the context.
    _memory.rollbackonly();
}


// We are rollback the child process 
void xrun::rollback(void) {
  DEBUG("\n\nNOW ROLLING BACK\n\n\n");
  DEBUG("\n\nNOW ROLLING BACK\n\n\n");
  // If this is the first time to rollback,
  // then we should rollback now.
  if(global_hasRollbacked()) {
    DEBUG("HAS rollback, now exit\n");
    rollbackandstop();
    abort();
  }


  // Rollback all memory before rolling back the context.
  _memory.rollback();
  
  // We must prepare the rollback, for example, if multiple
  // threads is existing, we must initiate the semaphores for threads
  // Also, we should setup those synchronization event list 
  _thread.prepareRollback();

  //while(1);
  DEBUG("\n\nset rollback\n\n\n");

  // Now we are going to rollback
  startRollback();

  assert(0);
  // FIXME: how to start those temporary stopped threads.
  // Setcontext
}

/// @brief Start a new epoch.
void xrun::epochBegin (void) {
  DEBUG("getpid %d: xrun::epochBegin. \n", getpid());

  threadmap::aliveThreadIterator i;
  for(i = threadmap::getInstance().begin(); 
      i != threadmap::getInstance().end(); i++)
  {
    thread_t * thread = i.getThread();

    if(thread != current) {
      lock_thread(thread);
        
      if(thread->hasJoined == true) {
        thread->status = E_THREAD_EXITING;
        Real::pthread_cond_signal()(&thread->cond);
        unlock_thread(thread);
        Real::pthread_join()(thread->self, NULL);
      }
      else {
        unlock_thread(thread);
      }
    }
  }

  // Saving the context of the memory.
  xthread::epochBegin();
  _memory.epochBegin();
  xthread::getInstance().runDeferredSyncs();
  
  // Now waken up all other threads then threads can do its cleanup.
  DEBUG("getpid %d: xrun::epochBegin, wakeup others. \n", getpid());
  global_epochBegin();

#ifdef HANDLE_SYSCALL
  // Start the new epoch for current thread 
  syscalls::getInstance().handleEpochBegin();
#endif

  DEBUG("getpid %d: xrun::epochBegin\n", getpid());
  
  // Save the context of this thread
  saveContext(); 
}

/// @brief End a transaction, aborting it if necessary.
void xrun::epochEnd (bool endOfProgram) {
  //DEBUG("in the end of an epoch with endOfProgram %d\n", endOfProgram);
  // Tell other threads to stop and save context.
  stopAllThreads();

#ifdef DEBUG_ROLLBACK
  //rollback();
  //assert(0);
#endif

#ifdef DETECT_OVERFLOW
  bool hasOverflow = _memory.checkHeapOverflow();
#endif

#ifdef DETECT_MEMORY_LEAKAGE
  bool hasMemoryLeak;
  if(endOfProgram) {
  //  DEBUG("DETECTING MEMORY LEAKABE in the end of program!!!!\n");
    hasMemoryLeak = leakcheck::getInstance().doFastLeakCheck(_memory.getHeapBegin(), _memory.getHeapEnd()); 
  }
  else {
   // DEBUG("DETECTING MEMORY LEAKABE inside a program!!!!\n");
    hasMemoryLeak = leakcheck::getInstance().doSlowLeakCheck(_memory.getHeapBegin(), _memory.getHeapEnd());
  }
#endif

#ifndef EVALUATING_PERF
  // First, attempt to commit.
  #if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKAGE)
  if(hasOverflow || hasMemoryLeak) {
    _memory.cleanupFreeList();
    rollback();
  }
  else {
  #elif defined(DETECT_OVERFLOW)
  if(hasOverflow) {
    _memory.cleanupFreeList();
    rollback();
  }
  else {
  #elif defined(DETECT_MEMORY_LEAKAGE)
  if(hasMemoryLeak) {
    _memory.cleanupFreeList();
    rollback();
  }
  else {
  #endif
#else
    _memory.cleanupFreeList();
#endif
    _memory.freeAllObjects();
    DEBUG("getpid %d: xrun::epochEnd without overflow\n", getpid());
    //DEBUG("getpid %d: xrun::epochEnd without overflow\n", getpid());
    syscalls::getInstance().epochEndWell();
    xthread::getInstance().epochEndWell();
#ifndef EVALUATING_PERF
  #if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKAGE)
  }
  #endif
#endif
}

bool isThreadSafe(thread_t * thread) {
  return thread->isSafe;
}

#ifdef DETECT_USAGE_AFTER_FREE
void xrun::finalUAFCheck(void) {
  threadmap::aliveThreadIterator i;
  // Check all threads' quarantine list
  for(i = threadmap::getInstance().begin(); 
      i != threadmap::getInstance().end(); i++)
  {
    thread_t * thread = i.getThread();
    
    if(thread->qlist.finalUAFCheck()) {
      rollback();
    }
  }

  // Check freelist
  if(freelist::getInstance().checkUAF()) {
    rollback();
  }
}
#endif

void xrun::stopAllThreads(void) {
  threadmap::aliveThreadIterator i;
  int waiters = 0;
  int count = 0;
  /* According to description of pthread_kill:
     One comment: The PID field in the TCB can temporarily be changed
     (in fork).  But this must not affect this code here.  Since this
     function would have to be called while the thread is executing
     fork, it would have to happen in a signal handler.  But this is
     no allowed, pthread_kill is not guaranteed to be async-safe.  */
  //threadmap::getInstance().traverseAllThreads();
  global_checkWaiters();

  // Used to tell other threads about normal epoch end because of one have to commit.
  // pthread_kill can not support additional signal value except signal number
  global_setEpochEnd();

  // In order to avoid this problem, we may avoid the thread spawning in this phase.
  global_lock();

  //DEBUG("EPOCHEBD:Current thread at %p self %p\n", current, pthread_self());
  DEBUG("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^EPOCHEBD:Current thread at %p THREAD%d self %p. Stopping other threads\n", current, current->index, (void*)pthread_self());
  for(i = threadmap::getInstance().begin(); 
      i != threadmap::getInstance().end(); i++)
  {
    thread_t * thread = i.getThread();

    //DEBUG("in epochend, thread%d mutex is %p\n", thread->index, &thread->mutex);
    if(thread != current) {
      lock_thread(thread);
      // If the thread's status is already at E_THREAD_WAITFOR_REAPING
      if(thread->status != E_THREAD_WAITFOR_REAPING) {
        if(thread->isSafe) {
        // If the thread is in cond_wait or barrier_wait, 
          DEBUG("in epochend, stopping thread %p self %p status %d\n", thread, (void*)thread->self, thread->status);
          //DEBUG("in epochend, thread %p self %p status %d\n", thread, thread->self, thread->status);
          waiters++;
          Real::pthread_kill()(thread->self, SIGUSR2);
        }
        else {
          //DEBUG("NOTSAFE!!! Thread %p self %p status %d\n", thread, thread->self, thread->status);
          thread->waitSafe = true;
          while(!thread->isSafe) {
            wait_thread(thread);
          }
          if(thread->status != E_THREAD_WAITFOR_REAPING) {
            Real::pthread_kill()(thread->self, SIGUSR2);
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

bool isNewThread(void) {
  return current->isNewlySpawned;
}

void jumpToFunction(ucontext_t * cxt, unsigned long funcaddr) {
    DEBUG("%p: inside signal handler %p.\n", (void*)pthread_self(), (void*)cxt->uc_mcontext.gregs[REG_RIP]);
    //selfmap::getInstance().printCallStack(NULL, NULL, true);
    cxt->uc_mcontext.gregs[REG_RIP] = funcaddr;
}

// Handling the signal SIGUSR2
/* We are using the SIGUSR2 to notify other threads something. 
  
 */
void xrun::sigusr2Handler(int signum, siginfo_t * siginfo, void * context) {
  // Check what is current status of the system.
  // If we are in the end of an epoch, then we save the context somewhere since
  // current thread is going to stop execution in order to commit or rollback.
  assert(global_isEpochEnd() == true);

  //printf("Thread %p in sigusr2Handler %d\n", pthread_self(), __LINE__);
  // Wait for notification from the commiter 
  global_waitForNotification();

  // Check what is the current phase: rollback or resume
  if(global_isEpochBegin()) {
    DEBUG("%p wakeup from notification.\n", (void*)pthread_self());
  // DEBUG("%p reset contexts~~~~~\n", pthread_self());
    xthread::epochBegin();
    xthread::getInstance().saveSpecifiedContext((ucontext_t *)context);   
    syscalls::getInstance().handleEpochBegin();
   // xthread::getInstance().resetContexts();
    // NOTE: we do not need to reset contexts if we are still inside the signal handleer
    // since the exiting from signal handler can do this automatically.
  } 
  else {
    DEBUG("epochBegin %d rollback %d\n", global_isEpochBegin(), global_isRollback());
    assert(global_isRollback() == true);
  
    if(isNewThread()) {
      lock_thread(current);

      // Waiting for the waking up from the its parent thread
      while(current->status != E_THREAD_ROLLBACK) {
        Real::pthread_cond_wait()(&current->cond, &current->mutex);
      }
        
      unlock_thread(current);
    }
    // Rollback inside signal handler is different
    xthread::getInstance().rollbackInsideSignalHandler((ucontext_t *)context);
  }
  // Jump to a function and wait for the instruction of the committer thread.
  //jumpToFunction((ucontext_t *)context, (unsigned long)afterSignalHandler);
}
