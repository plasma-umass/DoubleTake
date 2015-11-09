/*
 * @file   xthread.cpp
 * @brief  Handle Thread related information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <sys/types.h>

#include "xthread.hh"

#include "globalinfo.hh"
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
#include "xmemory.hh"

// Global lock used when joining and exiting a thread.
// threadmap::threadHashMap threadmap::_xmap;
// list_t threadmap::_alivethreads;

// bool xthread::_isRollbackPhase;
// list_t xthread::_deadSyncVars;
__thread DT::Thread* current;
// threadmap::threadHashMap threadmap::_xmap;
list_t threadmap::_alivethreads;

int getThreadIndex() {
  // if current hasn't been initialized, we must be in the xrun
  // constructor, in which case we know we are the first and only
  // thead.
  if (!current)
    return 0;
  return current->index;
}

int xthread::getThreadIndex() const {
  return ::getThreadIndex();
}

char* xthread::getCurrentThreadBuffer() {
  int index = getThreadIndex();

  return _thread.getThreadBuffer(index);
}

void xthread::invokeCommit() {
  xrun::getInstance().epochEnd(false);
	PRINF("invokeCommit after epochEnd\n");
  xrun::getInstance().epochBegin();
}

// Each thread should 
void xthread::epochBegin(DT::Thread *thread) {
	
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

void xthread::rollbackOtherThreads() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    DT::Thread* thread = i.getThread();
      
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
    DT::Thread* thread = i.getThread();

    // If we found the entry, remove this entry from the list.
    destroyThreadSemaphore(thread);
  }
}

// Initialize the semaphore for  specified thread
void xthread::destroyThreadSemaphore(DT::Thread* thread) {
  semaphore* sema = &thread->sema;

  // We initialize the semaphore value to 0.
  sema->destroy();
}

// Initialize the semaphore for  specified thread
void xthread::initThreadSemaphore(DT::Thread* thread) {
    semaphore* sema = &thread->sema;

    PRINF("INITSEMA: THREAD%d at %p sema %p\n", thread->index, (void *)thread, (void *)sema);
    PRINF("INITSEMA: THREAD%d at %p sema %p\n", thread->index, (void *)thread, (void *)sema);
    // We initialize the semaphore value to 0.
    sema->init((unsigned long)thread->self, 1, 0);
}

void xthread::rollback() {
  PRINF("calling syscalls::prepareRollback\n");
  PRINF("calling threadmap::prepareRollback\n");

	rollbackOtherThreads();

  PRINF("calling xsync::prepareRollback\n");
  PRINF("before calling sync::prepareRollback\n");
  // Update the semaphores and synchronization entry
  _sync.prepareRollback();
	xsync::prepareEventListRollback(_spawningList);
	PRINF("after calling sync::prepareRollback\n");
}

void xthread::wakeupOldWaitingThreads() {
	threadmap::aliveThreadIterator i;

	for(i = threadmap::getInstance().begin(); i != threadmap::getInstance().end(); i++) {
    DT::Thread* thread = i.getThread();
 
		// Currently, we only care about those old threads since 
		// the parent will wakeup those newly spawned threads appropriately 
		if(thread->isNewlySpawned != true) {
			PRINF("wakeup thread %d at wakeupOldWaitingThreads\n", thread->index);    
    	if(thread->status == E_THREAD_WAITFOR_REAPING) {
    		// If the thread is already at E_THREAD_WAITFOR_REAPING and it not a newly spawned thread,
				// then we should wake this thread up immediately
				thread->status = E_THREAD_ROLLBACK;
				thread->signal();
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

bool xthread::isThreadSafe(DT::Thread * thread) {
	return __atomic_load_n(&thread->isSafe, __ATOMIC_SEQ_CST);
}

bool xthread::addQuarantineList(void* ptr, size_t sz) {
  return current->qlist.addFreeObject(ptr, sz);
}

void xthread::rollbackCurrent() {
	// Setting the current status
  current->status = E_THREAD_RUNNING;

  current->qlist.restore();
  PRINF("xthread::rollback now");
  // Recover the context for current thread.
  current->context.rollback();
}

/// Handling the specific thread event.
void xthread::thread_exit(void*) {
  // FIXME later.
  //  abort();
}

void xthread::registerInitialThread(xmemory* memory) {
  DT::Thread *t = _thread.allocThread();
  if (t == nullptr) {
    FATAL("couldn't allocThreadIndex");
  }

  current = t;

  insertAliveThread(current, pthread_self());

  current->initialize(true, memory);
  current->isNewlySpawned = false;
}



// In order to improve the speed, those spawning operations will do in
// a batched way. Everything else will be stopped except this spawning
// process.  All newly spawned children will also wait for the
// notification of the parent.  SO only the first time, the thread
// will wait on fence.  To guarantee the correctness, those newly
// spawned threads will issue an epochBegin() to discard those
// possibly polluted pages.  For the parent, because no one is running
// when spawnning, so there is no need to call epochBegin().
int xthread::thread_create(pthread_t* tid, const pthread_attr_t* attr, threadFunction* fn, void* arg) {
  int result;

  PRINF("process %d is before thread_create now\n", current->index);
  if(!doubletake::inRollback) {
    // Lock and record
    xrun::getInstance().epochEnd(false);

    // Allocate a global thread index for current thread.
    DT::Thread *child = _thread.allocThread();
    assert(child != nullptr);

    if (attr) {
      int detachState;
      pthread_attr_getdetachstate(attr, &detachState);

      // Check whether the thread is detached or not?
      if(detachState == PTHREAD_CREATE_DETACHED) {
        child->isDetached = true;
      }
    }

    child->parent = current;
    child->startRoutine = fn;
    child->startArg = arg;
    child->status = E_THREAD_STARTING;
    child->creationEpoch = doubletake::epochID();

    child->lock();

    // Now we set the joiner to NULL before creation.
    // It is impossible to let newly spawned child to set this correctly since
    // the parent may already sleep on that.
    child->joiner = NULL;

    PRINF("thread creation with index %d\n", child->index);
    // Now we are going to record this spawning event.
    disableCheck();
    result = Real::pthread_create(tid, attr, xthread::startThread, (void*)child);
    enableCheck();
    if (result != 0) {
      FATAL("thread creation failed with errno %d -- %s", errno, strerror(errno));
    }

    while (!child->isSafe)
      child->wait();
    child->unlock();

    // Record spawning event
    _spawningList->recordSyncEvent(E_SYNC_SPAWN, result);
    _sysrecord.recordCloneOps(result, *tid);

    insertAliveThread(child, *tid);

    xrun::getInstance().epochBegin();
  } else {
    result = _sync.peekSyncEvent(_spawningList);
    PRINF("process %d is before thread_create, result %d\n", current->index, result);

    _sysrecord.getCloneOps(tid, &result);
    PRINF("process %d in creation, result %d\n", current->index, result);
    if(result == 0) {
      waitSemaphore();
      PRINF("process %d is after waitsemaphore, thread %lx\n", current->index, *tid);

      // Wakeup correponding thread, now they can move on.
      DT::Thread* thread = getThread(*tid);

      // Wakeup corresponding thread
      thread->joiner = NULL;
      // Check the thread's status
      if(thread->status == E_THREAD_WAITFOR_REAPING) {
        thread->status = E_THREAD_ROLLBACK;
        thread->signal();
      }
      else if(thread->status == E_THREAD_COND_WAITING || thread->status == E_THREAD_JOINING) {
        PRINF("Waken up thread %d with status %d condwait %p in thread_creation\n",
              thread->index, thread->status, (void *)thread->condwait);
        thread->status = E_THREAD_ROLLBACK;
        Real::pthread_cond_signal(thread->condwait);
      }
      //FIXME Tongping
      //while(1);
    }
    // Whenever we are calling __clone, then we can ask the thread to rollback?
    // Update the events.
    PRINF("#############process %d before updateSyncEvent now\n", current->index);
    updateSyncEvent(_spawningList);
    PRINF("#############process %d after updateSyncEvent now\n", current->index);
  }

  return result;
}

/// @brief Wait for a thread to exit.
int xthread::thread_join(pthread_t joinee, void** result) {

  // FIXME: if the thread has already terminated, return immediately.

  DT::Thread *thread = getThread(joinee);
  assert(thread != NULL);

  PRINF("thread %d is joining thread %d\n", current->index, thread->index);

  // FIXME: this logic needs some work

  PRINF("main thread is joining thread %d with status %d\n", thread->index, thread->status);
  while(thread->status != E_THREAD_WAITFOR_REAPING) {
    // Set the joiner to current thread
    thread->joiner = current;
    thread->wait();
  }

  // Now mark this thread's status so that the thread can be reaped.
  thread->hasJoined = true;

  // Actually, we should get the result from corresponding thread
  if(result) {
    *result = thread->result;
  }

  // Defer the reaping of this thread for memory deterministic usage.
  if(deferSync((void *)thread, E_SYNCVAR_THREAD)) {
    PRINF("Before reap dead threads!!!!\n");
    invokeCommit();
    PRINF("After reap dead threads!!!!\n");
  }

  return 0;
}

/// @brief Detach a thread
int xthread::thread_detach(pthread_t ptid) {
  // TODO: Try to check whether thread is empty or not?
  DT::Thread *thread = getThreadInfo(ptid);
  assert(thread != NULL);

  thread->lock();
  thread->isDetached = true;
  thread->unlock();

  abort();
}

  /// @brief Do a pthread_cancel
int xthread::thread_cancel(pthread_t thread) {
  int retval;
  invokeCommit();
  retval = Real::pthread_cancel(thread);
  if(retval == 0) {
    _thread.cancelAliveThread(thread);
  }
  return retval;
}


int xthread::thread_kill(pthread_t thread, int sig) {
  return Real::pthread_kill(thread, sig);
}


void* xthread::startThread(void* arg) {
  void* result = NULL;
  current = (DT::Thread *)arg;
  current->initialize(false, nullptr);
  xrun::getInstance().installSignalHandlers();

  current->lock();

  // Now current thread is safe to be interrupted.
  setThreadSafe();
  current->signal();
  current->unlock();

  current->context.saveCurrent();
  doubletake::unblockEpochSignal();

  PRINF("thread %p self %p after thread register now.", (void *)current, (void *)current->self);

  doubletake::waitForEpochComplete(current->creationEpoch);

  // We actually get those parameter about new created thread
  // from the TLS storage.
  result = current->startRoutine(current->startArg);
  PRINF("result %p of calling startRoutine %p on thread %d\n", (void *)result, (void *)current->startRoutine, current->index);
  // Insert dead list now so that the corresponding entry can be cleaned up if
  // there is no need to rollback.

  current->result = result;
  current->status = E_THREAD_WAITFOR_REAPING;

  // Only check the joiner when the thread is not deatached.
  if(!isThreadDetached()) {
    // Check the state of joiner.
    if(current->joiner) {
      assert(current->joiner->status == E_THREAD_JOINING);
      PRINF("Waking up the joiner %p!!!\n", (void*)current->joiner->self);
      // Now we can wakeup the joiner.
      current->signal();
    }
  } else {
    PRINF("Thread is detached!!!\n");
  }

  // At commit points, if no heap overflow is detected, then the thread
  // should set the status to E_THREAD_EXITING, otherwise it should
  // be set to E_THREAD_ROLLBACK
  while(current->status != E_THREAD_EXITING && current->status != E_THREAD_ROLLBACK) {
    current->wait();
  }

  // What to do in the end of a thread?
  // It can only have two status, one is to rollback and another is to exit successfully.
  if(current->status == E_THREAD_ROLLBACK) {
    PRINF("THREAD%d (at %p) is wakenup and plan to rollback\n", current->index, (void *)current);
    current->unlock();

    // Rollback: has the possible race conditions. FIXME
    // Since it is possible that we copy back everything after the join, then
    // Some thread data maybe overlapped by this rollback.
    // Especially those current->joiner, then we may have a wrong status.
    PRINF("THREAD%d (at %p) is rollngback now\n", current->index, (void *)current);
    xrun::getInstance().thread()->rollbackCurrent();

    // We will never reach here
    assert(0);
  } else {
    assert(current->status == E_THREAD_EXITING);
    PRINF("THREAD%d (at %p) is wakenup and plan to exit now\n", current->index, (void *)current);
    current->unlock();
  }
  return result;
}
