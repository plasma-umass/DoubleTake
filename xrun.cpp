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

void xrun::startRollback(void) {
  // Now we are going to rollback. Wakup all threads
  // waiting on the shared conditional variable
  globalinfo::getInstance().rollback();

  PRWRN("Starting rollback for other threads\n");

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

    PRDBG("\n\nNOW ROLLING BACK\n\n\n");
  
    // Rollback all memory before rolling back the context.
    _memory.rollbackonly();
}

    // We are rollback the child process 
void xrun::rollback(void) {
  PRDBG("\n\nNOW ROLLING BACK\n\n\n");
  fprintf(stderr, "\n\nNOW ROLLING BACK\n\n\n");
  // If this is the first time to rollback,
  // then we should rollback now.
  if(globalinfo::getInstance().hasRollbacked()) {
    PRDBG("HAS rollback, now exit\n");
    rollbackandstop();
    EXIT;
  }

  // We must prepare the rollback, for example, if multiple
  // threads is existing, we must initiate the semaphores for threads
  _thread.prepareRollback();

  // Rollback all memory before rolling back the context.
  _memory.rollback();

  PRDBG("\n\nset rollback\n\n\n");

  // Now we are going to rollback
  startRollback();

  assert(0);
  // FIXME: how to start those temporary stopped threads.
  // Setcontext
}

/// @brief Start a new epoch.
void xrun::epochBegin (void) {
  PRDBG("getpid %d: xrun::epochBegin. changed\n", getpid());

  threadmap::aliveThreadIterator i;
  for(i = threadmap::getInstance().begin(); 
      i != threadmap::getInstance().end(); i++)
  {
    thread_t * thread = i.getThread();

    if(thread != current) {
      // Wakenup those newly spawned threads.
      WRAP(pthread_mutex_lock)(&thread->mutex);
      WRAP(pthread_cond_signal)(&thread->cond);
        
      if(thread->hasJoined == true) {
        thread->status = E_THREAD_EXITING;
        WRAP(pthread_mutex_unlock)(&thread->mutex);
        WRAP(pthread_join)(thread->self, NULL);
      }
      else {
        WRAP(pthread_mutex_unlock)(&thread->mutex);
      }
    }
  }

  // Saving the context of the memory.
  _memory.epochBegin();
  xthread::getInstance().runDeferredSyncs();
  
  // Now waken up all other threads then threads can do its cleanup.
  globalinfo::getInstance().epochBegin();

  // Start the new epoch for current thread 
  syscalls::getInstance().cleanupRecords();

  PRDBG("getpid %d: xrun::epochBegin\n", getpid());
  // Save the context of this thread
  saveContext(); 
}

/// @brief End a transaction, aborting it if necessary.
void xrun::epochEnd (void) {
  // Tell other threads to stop and save context.
  stopAllThreads();

#ifdef DEBUG_ROLLBACK
  //rollback();
  //assert(0);
#endif

  bool hasOverflow = _memory.checkHeapOverflow();
 
  // First, attempt to commit.
  if(hasOverflow) {
    rollback();
  }
  else {
    PRDBG("getpid %d: xrun::epochEnd without overflow\n", getpid());
    //fprintf(stderr, "getpid %d: xrun::epochEnd without overflow\n", getpid());
    syscalls::getInstance().epochEndWell();
    xthread::getInstance().epochEndWell();
  }
}

bool isThreadSafe(thread_t * thread) {
  return thread->isSafe;
}

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

  globalinfo::getInstance().checkWaiters();

  // In order to avoid this problem, we may avoid the thread spawning in this phase.
  xthread::getInstance().global_lock();

  // Used to tell other threads about normal epoch end because of one have to commit.
  // pthread_kill can not support additional signal value except signal number
  globalinfo::getInstance().setEpochEnd();

  fprintf(stderr, "EPOCHEBD:Current thread at %p self %p\n", current, pthread_self());
#if 1  
  for(i = threadmap::getInstance().begin(); 
      i != threadmap::getInstance().end(); i++)
  {
    thread_t * thread = i.getThread();

//    fprintf(stderr, "in epochend, thread%d mutex is %p\n", thread->index, &thread->mutex);
    if(thread != current) {
      WRAP(pthread_mutex_lock)(&thread->mutex);
      // If the thread's status is already at E_THREAD_WAITFOR_REAPING
      if(thread->status != E_THREAD_WAITFOR_REAPING) {
        if(thread->isSafe) {
        // If the thread is in cond_wait or barrier_wait, 
          PRDBG("in epochend, stopping thread %p self %p status %d\n", thread, thread->self, thread->status);
      //  fprintf(stderr, "in epochend, thread %p self %p status %d\n", thread, thread->self, thread->status);
          WRAP(pthread_kill)(thread->self, SIGUSR2);
        }
        // Else, if thread is not safe, the thread can wait by itself.
        waiters++;
      }
    
      WRAP(pthread_mutex_unlock)(&thread->mutex);
    }
    count++;
  }

  if(waiters != 0) {
    globalinfo::getInstance().setWaiters(waiters);
  }

  xthread::getInstance().global_unlock();
   
  // Wait until all other threads are stopped.  
  if(waiters != 0) {
    globalinfo::getInstance().waitThreadsStops();
  }
 // usleep(500);
#else
  xthread::getInstance().global_unlock();
#endif
}

bool isNewThread(void) {
  return current->isNewlySpawned;
}

/**
   Those threads being signaled are going to call this function.
   Notice that it is outside the signal handler so that we don't worry 
   about the exiting of signal handler.
   There are two possible results after signal handler.
     1. Continue the execution when no overflow. So we may have to cleanup
        those recording information at first. Then current thread can continue
        the execution. 
        In this case, we copy the newContext to oldContext and set myself to newContext.
     2. When there is some overflow, we have to rollback. We will not cleanup
        those recording information. In this case, we will discard the newContext
        and set the context to oldContext. 
*/
void afterSignalHandler(void)  {
#if MYDEBUG
  //PRDBG("%p after signal handler.\n", pthread_self());
  // Wait on the global conditional variable.
  globalinfo::getInstance().waitForNotification();

  // Check what is the current phase: rollback or resume
  if(globalinfo::getInstance().isEpochBegin()) {
    PRDBG("%p wakeup from notification.\n", pthread_self());
  // PRDBG("%p reset contexts~~~~~\n", pthread_self());
    syscalls::getInstance().cleanupRecords();
  
    xthread::getInstance().resetContexts();
  }
  else {
    assert(globalinfo::getInstance().isRollback() == true);
  
    if(isNewThread()) {
      WRAP(pthread_mutex_lock)(&current->mutex);

      // Waiting for the waking up from the committing thread
      while(current->status != E_THREAD_ROLLBACK) {
        WRAP(pthread_cond_wait)(&current->cond, &current->mutex);
      }
        
      //fprintf(stderr, "THREAD%d (at %p) is wakenup\n", current->index, current);
      WRAP(pthread_mutex_unlock)(&current->mutex);
    }
    xthread::getInstance().rollback();
  }
#else
    xthread::getInstance().resetContexts();
#endif
}

void jumpToFunction(ucontext_t * cxt, unsigned long funcaddr) {
    PRDBG("%p: inside signal handler %lx.\n", pthread_self(), cxt->uc_mcontext.gregs[REG_RIP]);
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
  assert(globalinfo::getInstance().isEpochEnd() == true);

  // Wait for notification from the commiter 
  globalinfo::getInstance().waitForNotification();

  // Check what is the current phase: rollback or resume
  if(globalinfo::getInstance().isEpochBegin()) {
    PRDBG("%p wakeup from notification.\n", pthread_self());
  // PRDBG("%p reset contexts~~~~~\n", pthread_self());
    xthread::getInstance().saveSpecifiedContext((ucontext_t *)context);   
    syscalls::getInstance().cleanupRecords();
   // xthread::getInstance().resetContexts();
  } 
  else {
    assert(globalinfo::getInstance().isRollback() == true);
  
    if(isNewThread()) {
      WRAP(pthread_mutex_lock)(&current->mutex);

      // Waiting for the waking up from the committing thread
      while(current->status != E_THREAD_ROLLBACK) {
        WRAP(pthread_cond_wait)(&current->cond, &current->mutex);
      }
        
      //fprintf(stderr, "THREAD%d (at %p) is wakenup\n", current->index, current);
      WRAP(pthread_mutex_unlock)(&current->mutex);
    }
    // Rollback inside signal handler is different
    xthread::getInstance().rollbackInsideSignalHandler((ucontext_t *)context);
  }
  // Jump to a function and wait for the instruction of the committer thread.
  // Set the context to handleSegFault
  //jumpToFunction((ucontext_t *)context, (unsigned long)afterSignalHandler);
}
