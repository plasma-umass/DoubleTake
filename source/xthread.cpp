/*
 * @file   xthread.cpp
 * @brief  Handle Thread related information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xthread.hh"

#include "globalinfo.hh"
#include "internalsyncs.hh"
#include "list.hh"
#include "log.hh"
#include "quarantine.hh"
#include "recordentries.hh"
#include "threadinfo.hh"
#include "threadmap.hh"
#include "threadstruct.hh"
#include "xrun.hh"
#include "syscalls.hh"
#include "xsync.hh"

// Global lock used when joining and exiting a thread.
// threadmap::threadHashMap threadmap::_xmap;
// list_t threadmap::_alivethreads;

// bool xthread::_isRollbackPhase;
// list_t xthread::_deadSyncVars;
__thread thread_t* current;
// threadmap::threadHashMap threadmap::_xmap;
list_t threadmap::_alivethreads;

int getThreadIndex() {
  if(!global_isInitPhase()) {
    return current->index;
  } else {
    return 0;
  }
}

char* getThreadBuffer() {
  int index = getThreadIndex();

  return threadinfo::getInstance().getThreadBuffer(index);
}

void xthread::invokeCommit() {
  xrun::getInstance().epochEnd(false);
	PRINT("invokeCommit after epochEnd\n");
  xrun::getInstance().epochBegin();
}

// Each thread should 
void xthread::epochBegin(thread_t * thread) {
	// Now we should not have the pending synchronization events.	
	listInit(&thread->pendingSyncevents);

	// Handle the quarantine list of memory.
  thread->qlist.restore();

	//PRINF("Cleanup all synchronization events for this thread\n");
	// cleanup the synchronization events of this thread
  thread->syncevents.cleanup();

	// We should cleanup the syscall events for this thread.
	SysRecord::epochBegin(thread);	
	//PRINF("Cleanup all synchronization events for this thread done\n");
}

void xthread::prepareRollbackAlivethreads() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();
      
		// Initialize the semaphore for this thread.
    initThreadSemaphore(thread);

    // Set the entry of each thread to the first synchronization event.
   	thread->syscalls.prepareRollback();
	  thread->syncevents.prepareRollback();
		SysRecord::prepareRollback(thread);	
  }
}
  
/**
  destroy all semaphores:
*/
void xthread::destroyAllSemaphores() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();

    // If we found the entry, remove this entry from the list.
    destroyThreadSemaphore(thread);
  }
}

// Initialize the semaphore for  specified thread
void xthread::destroyThreadSemaphore(thread_t* thread) {
  semaphore* sema = &thread->sema;

  // We initialize the semaphore value to 0.
  sema->destroy();
}

// Initialize the semaphore for  specified thread
void xthread::initThreadSemaphore(thread_t* thread) {
    semaphore* sema = &thread->sema;

    PRINF("INITSEMA: THREAD%d at %p sema %p\n", thread->index, thread, sema);
    PRINF("INITSEMA: THREAD%d at %p sema %p\n", thread->index, thread, sema);
    // We initialize the semaphore value to 0.
    sema->init((unsigned long)thread->self, 1, 0);
}

void xthread::prepareRollback() {
  PRINF("calling syscalls::prepareRollback\n");
  PRINT("calling threadmap::prepareRollback\n");

	prepareRollbackAlivethreads();

  PRINF("calling xsync::prepareRollback\n");
  PRINT("before calling sync::prepareRollback\n");
  // Update the semaphores and synchronization entry
  _sync.prepareRollback();
	PRINT("after calling sync::prepareRollback\n");

	// Setting the phase to rollback
	global_setRollback(); 
 
	// Now it is time to wake up those waiting threads
	// if they are not newly spawned in this epoch.
	wakeupOldWaitingThreads();

	// Wakeup those threads that are waiting on the global waiters.
	global_wakeup();	
}

void xthread::wakeupOldWaitingThreads() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    thread_t* thread = i.getThread();
 
		// Currently, we only care about those old threads since 
		// the parent will wakeup those newly spawned threads appropriately 
		if(thread->isNewlySpawned != true) {
			PRINT("wakeup thread %d at wakeupOldWaitingThreads\n", thread->index);    
    	if(thread->status == E_THREAD_WAITFOR_REAPING) {
    		// If the thread is already at E_THREAD_WAITFOR_REAPING and it not a newly spawned thread,
				// then we should wake this thread up immediately
				thread->status = E_THREAD_ROLLBACK;
				signal_thread(thread);
    	}	
 			else if (thread->status == E_THREAD_COND_WAITING || thread->status == E_THREAD_JOINING) {
				thread->status = E_THREAD_ROLLBACK;
				Real::pthread_cond_signal(thread->condwait);
			}
		}
	}		
}

void xthread::setThreadSafe() {
  __atomic_store_n(&current->isSafe, true, __ATOMIC_SEQ_CST);
//  signal_thread(current);
}

void xthread::setThreadUnsafe() {
  __atomic_store_n(&current->isSafe, false, __ATOMIC_SEQ_CST);
}

bool xthread::isThreadSafe(thread_t * thread) {
	return __atomic_load_n(&thread->isSafe, __ATOMIC_SEQ_CST);
}

bool xthread::addQuarantineList(void* ptr, size_t sz) {
  return current->qlist.addFreeObject(ptr, sz);
}

void xthread::checkRollbackCurrent() {
	// Check whether I should go to sleep or not.
  lock_thread(current);
  if(current->isNewlySpawned) {
    while(current->status != E_THREAD_ROLLBACK) {
    	wait_thread(current);
		}
	}
  unlock_thread(current);

	rollbackCurrent();
}

void xthread::rollbackCurrent() {
	// Setting the current status
  current->status = E_THREAD_RUNNING;

  current->qlist.restore();
  PRINT("xthread::rollback now\n");
  // Recover the context for current thread.
  restoreContext();
}
