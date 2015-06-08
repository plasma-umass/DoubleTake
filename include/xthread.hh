#if !defined(DOUBLETAKE_XTHREAD_H)
#define DOUBLETAKE_XTHREAD_H

/*
 * @file   xthread.h
 * @brief  Handling different kinds of synchronizations, like thread creation and exit,
 *         lock, conditional variables and barriers.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <syscall.h>
#include <ucontext.h>
#include <unistd.h>

#include <new>

#include "globalinfo.hh"
#include "internalheap.hh"
#include "internalsyncs.hh"
#include "list.hh"
#include "log.hh"
#include "real.hh"
#include "sysrecord.hh"
#include "selfmap.hh"
#include "semaphore.hh"
#include "synceventlist.hh"
#include "threadinfo.hh"
#include "threadmap.hh"
#include "threadstruct.hh"
#include "xcontext.hh"
#include "xdefines.hh"
#include "xsync.hh"

class xthread {

public:
  xthread() : _thread(threadinfo::getInstance()) {}

  // Actually, it is not an actual singleton.
  // Every process will have one copy. They can be used
  // to hold different contents specific to different threads.
  static xthread& getInstance() {
    static char buf[sizeof(xthread)];
    static xthread* xthreadObject = new (buf) xthread();
    return *xthreadObject;
  }

  void initialize() {
    _thread.initialize();

    // Initialize the syncmap and threadmap.
    _sync.initialize();
    threadmap::getInstance().initialize();

    // Initialize the global list for spawning operations
    void* ptr = ((void*)InternalHeap::getInstance().malloc(sizeof(SyncEventList)));
    _spawningList = new (ptr) SyncEventList(NULL, E_SYNC_SPAWN);

    // We do not know whether NULL can be support or not, so we use
    // fake variable name _spawningList here
    _sync.insertSyncMap(E_SYNCVAR_THREAD, (void*)_spawningList, _spawningList, _spawningList);

    // Register the first thread
    initialThreadRegister();
    current->isSafe = true;
    PRINF("Done with thread initialization");
  }

  void finalize() {
		destroyAllSemaphores(); 
	}

  // After an epoch is end and there is no overflow,
  // we should discard those record events since there is no
  // need to rollback anymore
  // tnrere are three types of events here.
  void epochEndWell() {
		// There is no need to run deferred synchronizations 
		// since we will do this in epochBegin();
		// Cleanup all synchronization events in the global list (mostly thread creations) and lists of 
		// different synchronization variables
		// FIXME: now it seems like that this function doesn't perform correctly.
    cleanSyncEvents();
  }

  // Register initial thread
  inline void initialThreadRegister() {
    int tindex = allocThreadIndex();

    if (tindex == -1) {
      return;
    }

    thread_t* tinfo = getThreadInfo(tindex);

    // Set the current to corresponding tinfo.
    current = tinfo;
    current->joiner = NULL;
    current->index = tindex;
    current->parent = NULL;

    insertAliveThread(current, pthread_self());

    // Setup tindex for initial thread.
    threadRegister(true);
    current->isNewlySpawned = false;
  }

  /// Handling the specific thread event.
  void thread_exit(void*) {
    // FIXME later.
    //  abort();
  }

  // In order to improve the speed, those spawning operations will do in
  // a batched way. Everything else will be stopped except this spawning process.
  // All newly spawned children will also wait for the notification of the parent.
  // SO only the first time, the thread will wait on fence.
  // To guarantee the correctness, those newly spawned threads will issue
  // an epochBegin() to discard those possibly polluted pages.
  // For the parent, because no one is running when spawnning, so there is no
  // need to call epochBegin().
  int thread_create(pthread_t* tid, const pthread_attr_t* attr, threadFunction* fn, void* arg) {
    int tindex;
    int result;

    if(!global_isRollback()) {
      // Lock and record
      global_lock();

      // Allocate a global thread index for current thread.
      tindex = allocThreadIndex();

      // This can be caused by two reasons:
      // First, xdefines::MAX_ALIVE_THREADS is too small.
      // Second, we haven't reaped the threads for a long time.
      if(tindex == -1) {
        REQUIRE(hasReapableThreads(), "No reapable threads");
        global_unlock();

        invokeCommit();

        global_lock();
        tindex = allocThreadIndex();
        REQUIRE(tindex != -1, "System can support %d threads", xdefines::MAX_ALIVE_THREADS);
        PRINF("AFTER commit now******* tindex %d\n", tindex);
      }

      // WRAP up the actual thread function.
      // Get corresponding thread_t structure.
      thread_t* children = getThreadInfo(tindex);

      children->isDetached = false;
      if(attr) {
        int detachState;
        pthread_attr_getdetachstate(attr, &detachState);

        // Check whether the thread is detached or not?
        if(detachState == PTHREAD_CREATE_DETACHED) {
          children->isDetached = true;
        }
      }

      children->parent = current;
      children->index = tindex;
      children->startRoutine = fn;
      children->startArg = arg;
      children->status = E_THREAD_STARTING;
      children->hasJoined = false;
      children->isSafe = false;

      // Now we set the joiner to NULL before creation.
      // It is impossible to let newly spawned child to set this correctly since
      // the parent may already sleep on that.
      children->joiner = NULL;

      PRINT("thread creation with index %d\n", tindex);
      // Now we are going to record this spawning event.
      disableCheck();
      result = Real::pthread_create(tid, attr, xthread::startThread, (void*)children);
      enableCheck();
      if(result != 0) {
        PRWRN("thread creation failed with errno %d -- %s\n", errno, strerror(errno));
        Real::exit(-1);
      }

      // Record spawning event
      _spawningList->recordSyncEvent(E_SYNC_SPAWN, result);
      _sysrecord.recordCloneOps(result, *tid);

      if(result == 0) {
        insertAliveThread(children, *tid);
      }

      global_unlock();

      if(result == 0) {
        // Waiting for the finish of registering children thread.
        lock_thread(children);

        while(children->status == E_THREAD_STARTING) {
          wait_thread(children);
          //     PRINF("Children %d status %d. now wakenup\n", children->index, children->status);
        }
        unlock_thread(children);
        //  	PRINF("Creating thread %d at %p self %p\n", tindex, children, (void*)children->self);
      }
    } else {
      PRINF("process %d is before thread_create now\n", current->index);
      result = _sync.peekSyncEvent(_spawningList);
      PRINT("process %d is before thread_create, result %d\n", current->index, result);

      _sysrecord.getCloneOps(tid, &result);
      PRINT("process %d in creation, result %d\n", current->index, result);
      if(result == 0) {
        waitSemaphore();
        PRINF("process %d is after waitsemaphore, thread %lx\n", current->index, *tid);

        // Wakeup correponding thread, now they can move on.
        thread_t* thread = getThread(*tid);

        // Wakeup corresponding thread
        thread->joiner = NULL;
       	// Check the thread's status
				if(thread->status == E_THREAD_WAITFOR_REAPING) {
        	thread->status = E_THREAD_ROLLBACK;
				 	signal_thread(thread);
				}
				else if(thread->status == E_THREAD_COND_WAITING || thread->status == E_THREAD_JOINING) {
        	PRINT("Waken up thread %d with status %d condwait %p in thread_creation\n", thread->index, thread->status, thread->condwait);
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

	inline void markThreadJoining(thread_t * thread) {
		lock_thread(current);
		current->status = E_THREAD_JOINING;
		current->condwait = &thread->cond;
		unlock_thread(current);
	}

	inline void unmarkThreadJoining() {
		checkRollback(NULL);
	}

  /// @brief Wait for a thread to exit.
  /// Should we record this? It is not necessary?? FIXME
  inline int thread_join(pthread_t joinee, void** result) {
    thread_t* thread = NULL;

    // Try to check whether thread is empty or not?
    thread = getThread(joinee);
    assert(thread != NULL);

		PRINT("main thread is joining thread %d\n", thread->index);
	
		// FIXME: is there a race here since we are using two locks.
		// what if the joinee has stopped for reaping, what will happen?
		// what if the checking happens when it is in the middle of waiting?	
		markThreadJoining(thread);
      
    lock_thread(thread);

		PRINT("main thread is joining thread %d with status %d\n", thread->index, thread->status);
    while(thread->status != E_THREAD_WAITFOR_REAPING) {
      // Set the joiner to current thread
      thread->joiner = current;

      // Wait for the joinee to wake me up
      wait_thread(thread);
    }

    // Now mark this thread's status so that the thread can be reaped.
    thread->hasJoined = true;

    // Actually, we should get the result from corresponding thread
    if(result) {
      *result = thread->result;
    }

    // Now we unlock and proceed
    unlock_thread(thread);

		// Check the status since it is possible to be waken up for rollback.
		unmarkThreadJoining();
	
		// Defer the reaping of this thread for memory deterministic usage.
		if(deferSync((void *)thread, E_SYNCVAR_THREAD)) {
			PRINF("Before reap dead threads!!!!\n");
			// deferSync may return TRUE if we have to reapDeadThreads now.
    	invokeCommit();
		}
 
    return 0;
  }

  /// @brief Detach a thread
  inline int thread_detach(pthread_t thread) {
    thread_t* threadinfo = NULL;

    // Try to check whether thread is empty or not?
    threadinfo = getThreadInfo(thread);

    assert(threadinfo != NULL);

    lock_thread(threadinfo);
    threadinfo->isDetached = true;
    unlock_thread(threadinfo);

    abort();
  }

  /// @brief Do a pthread_cancel
  inline int thread_cancel(pthread_t thread) {
    int retval;
    invokeCommit();
    retval = Real::pthread_cancel(thread);
    if(retval == 0) {
      threadinfo::getInstance().cancelAliveThread(thread);
    }
    return retval;
  }

  inline int thread_kill(pthread_t thread, int sig) { return Real::pthread_kill(thread, sig); }

  /// Save those actual mutex address in original mutex.
  int mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    // The synchronization here is totally broken: initializer should read state, then check
    // if init is needed. If so, then allocate and atomic compare exchange.

    if(!global_isRollback()) {
      // Allocate a mutex
      pthread_mutex_t* real_mutex =
          (pthread_mutex_t*)allocSyncEntry(sizeof(pthread_mutex_t), E_SYNC_MUTEX_LOCK);

      // Initialize the real mutex
      int result = Real::pthread_mutex_init(real_mutex, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(E_SYNCVAR_MUTEX, mutex, real_mutex, sizeof(pthread_mutex_t));

      return result;
    }

    return 0;
  }

  inline bool isInvalidSyncVar(void* realMutex) {
    return (((intptr_t)realMutex < xdefines::INTERNAL_HEAP_BASE) ||
                    ((intptr_t)realMutex >= xdefines::INTERNAL_HEAP_END)
                ? true
                : false);
  }

  int do_mutex_lock(void* mutex, thrSyncCmd synccmd) {
    int ret = 0;
    SyncEventList* list = NULL;

    if(!global_isRollback()) {
    	pthread_mutex_t* realMutex = NULL;
      realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
      //PRINF("do_mutex_lock after getSyncEntry %d realMutex %p\n", __LINE__, realMutex);
      if(isInvalidSyncVar(realMutex)) {
        mutex_init((pthread_mutex_t*)mutex, NULL);
        realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
      }

			// FIXME: mark the thread as unsafe.
      //  PRINF("pthread_self %lx: do_mutex_lock at line %d: mutex %p realMutex %p\n",
      // pthread_self(), __LINE__, mutex, realMutex);
      assert(realMutex != NULL);

  		setThreadUnsafe();
      switch(synccmd) {
      case E_SYNC_MUTEX_LOCK:
        ret = Real::pthread_mutex_lock(realMutex);
        break;

      case E_SYNC_MUTEX_TRY_LOCK:
        ret = Real::pthread_mutex_trylock(realMutex);
        break;

      default:
        break;
      }

      // Record this event
      list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
     	PRINT("Thread %d recording: mutex_lock at mutex %p realMutex %p list %p\n", current->index, mutex, realMutex, list);
      list->recordSyncEvent(E_SYNC_MUTEX_LOCK, ret);
     	//PRINF("Thread %d: mutex_lock at mutex %p realMutex %p list %p. Record done!!\n", current->index, mutex, realMutex, list);
    } else {
     	PRDBG("mutex_lock at mutex %p list %p\n", mutex, list);
      list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
      // PRINF("synceventlist get mutex at %p list %p\n", mutex, list);
     PRINT("REPLAY: Thread %d: mutex_lock at mutex %p list %p.\n", current->index, mutex, list);
      assert(list != NULL);

			/* Peek the synchronization event (first event in the thread), it will confirm the following things
			 1. Whether this event is expected event? If it is not, maybe it is caused by
			    a race condition. Maybe we should restart the rollback or just simply reported this problem.
			 2. Whether the first event is on the pending list, which means it is the thread's turn? 
					If yes, then the current signal thread should increment its semaphore.
			 3. Whether the mutex_lock is successful or not? If it is not successful, 
					no need to wait for the semaphore since there is no actual lock happens.
			*/ 
      ret = _sync.peekSyncEvent(list);
     	PRINF("REPLAY: After peek: Thread %d: mutex_lock at mutex %p list %p ret %d.\n", current->index, mutex, list, ret);
      if(ret == 0) {
        waitSemaphore();
      }

     	PRINF("REPLAY: After peekandwait: Thread %d: mutex_lock at mutex %p list %p ret %d.\n", current->index, mutex, list, ret);
     	PRDBG("mutex_lock at mutex %p list %p done\n", mutex, list);
     	//PRINF("mutex_lock at mutex %p list %p done!\n", mutex, list);
			_sync.advanceThreadSyncList();
    }
    return ret;
  }

  int mutex_lock(pthread_mutex_t* mutex) { return do_mutex_lock(mutex, E_SYNC_MUTEX_LOCK); }

  int mutex_trylock(pthread_mutex_t* mutex) { return do_mutex_lock(mutex, E_SYNC_MUTEX_TRY_LOCK); }

  int mutex_unlock(pthread_mutex_t* mutex) {
    int ret = 0;
    pthread_mutex_t* realMutex = NULL;

    if(!global_isRollback()) {
      realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
      ret = Real::pthread_mutex_unlock(realMutex);

			// Now the thread is safe to be interrupted.
  		setThreadSafe();
    } else {
      SyncEventList* list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
      PRDBG("mutex_unlock at mutex %p list %p\n", mutex, list);
      //	PRINF("mutex_unlock at mutex %p list %p\n", mutex, list);
      struct syncEvent* nextEvent = list->advanceSyncEvent();
      if(nextEvent) {
        _sync.signalNextThread(nextEvent);
      }
      PRDBG("mutex_unlock at mutex %p list %p done\n", mutex, list);
    }
    // WARN("mutex_unlock mutex %p\n", mutex);
    return ret;
  }

  // Add this event into the destory list.
  int mutex_destroy(pthread_mutex_t* mutex) {
    deferSync((void*)mutex, E_SYNCVAR_MUTEX);
    return 0;
  }

  ///// conditional variable functions.
  int cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr) {
    if(!global_isRollback()) {
      // Allocate a mutex
      pthread_cond_t* real_cond =
          (pthread_cond_t*)allocSyncEntry(sizeof(pthread_cond_t), E_SYNC_COND);

      // Initialize the real mutex
      int result = Real::pthread_cond_init(real_cond, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(E_SYNCVAR_COND, cond, real_cond, sizeof(pthread_cond_t));
      PRINT("cond_init for thread %d. cond %p realCond %p\n", current->index, cond, real_cond);

      return result;
    }

		return 0;
  }

  // Add this into destoyed list.
  void cond_destroy(pthread_cond_t* cond) { deferSync((void*)cond, E_SYNCVAR_COND); }


	// Mark whether 
	void markThreadCondwait(pthread_cond_t * cond) {
		lock_thread(current);
		assert(current->status == E_THREAD_RUNNING);
		current->status = E_THREAD_COND_WAITING;
		current->condwait = cond;
		unlock_thread(current);

		// Now thread is safe to be interrupted
		setThreadSafe();
		PRINT("markThreadCondwait on thread %d with cond %p\n", current->index, cond);		
	}

	/*
    The thread can be waken up on two different situations. 
		One is waken up through the application.
		Another is waken up by DoubleTake because of rollback.
	*/
	void unmarkThreadCondwait(pthread_mutex_t * mutex) {
		checkRollback(mutex);
		setThreadUnsafe();
	}


	void checkRollback(pthread_mutex_t * mutex) {
		PRINT("checkRollback on thread %d with cond %p mutex %p\n", current->index, current->condwait, mutex);		
		lock_thread(current);
		// Cleanup this thread
		current->condwait = NULL;
		
		if(current->status == E_THREAD_ROLLBACK) {
      PRINT("THREAD%d (status %d) is wakenup after cond_wait, plan to rollback\n", current->index, current->status);
			unlock_thread(current);

			if(mutex) {			
				// If we are holding the lock, release it before the rollback.
				Real::pthread_mutex_unlock(mutex);
			}
				
			// Time to rollback. 
			checkRollbackCurrent();
		}
		else {
      PRINT("THREAD%d (status %d) is wakenup after cond_wait with mutex %p\n", current->index, current->status, mutex);
			current->status = E_THREAD_RUNNING;
			unlock_thread(current);
		}
	}

  int cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
		return cond_wait_core(cond, mutex, NULL);
	}

	// Cond_timedwait: since we usually get the mutex before this. So there is
  // no need to check mutex any more.
  int cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec * abstime) {
		return cond_wait_core(cond, mutex, abstime);
	}

  // Condwait: since we usually get the mutex before this. So there is
  // no need to check mutex any more.
  int cond_wait_core(pthread_cond_t* cond, pthread_mutex_t* mutex, const struct timespec * abstime) {
    int ret;
    SyncEventList* list = NULL;
      
		PRINT("cond_wait_core for thread %d. cond %p mutex %p\n", current->index, cond, mutex);

    if(!global_isRollback()) {
      PRINT("cond_wait for thread %d. cond %p mutex %p\n", current->index, cond, mutex);
      pthread_mutex_t* realMutex = (pthread_mutex_t*)getSyncEntry(mutex);
      pthread_cond_t* realCond = (pthread_cond_t*)getSyncEntry(cond);
      assert(realMutex != NULL);

			if(isInvalidSyncVar(realCond)) {
        cond_init((pthread_cond_t*)cond, NULL);
        realCond = (pthread_cond_t*)getSyncEntry(cond);
      }
			assert(realCond != NULL);

			// Mark the status of this thread before going to sleep.
			markThreadCondwait(realCond);

      // Add the event into eventlist
			if(abstime == NULL) {
      	ret = Real::pthread_cond_wait(realCond, realMutex);
			}
			else {
      	ret = Real::pthread_cond_timedwait(realCond, realMutex, abstime);
			}
      PRINT("cond_wait wakeup for thread %d\n", current->index);
      
			// This thread exits cond_wait status	
			unmarkThreadCondwait(realMutex);

			// Record the waking up of conditional variable
			list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
      list->recordSyncEvent(E_SYNC_MUTEX_LOCK, ret);
		
    } else {
			list = getSyncEventList(mutex, sizeof(pthread_mutex_t));

			// Before the condWait, we will have to release the current lock.			
			struct syncEvent* nextEvent = list->advanceSyncEvent();
      if(nextEvent) {
        _sync.signalNextThread(nextEvent);
      }

			// Then we will have a mutex_lock after wakenup.
      ret = _sync.peekSyncEvent(list);

      if(ret == 0) {
        // Now waiting for the lock
        waitSemaphore();
      }
				
			_sync.advanceThreadSyncList();
    }

    return ret;
  }
  
  int cond_broadcast(pthread_cond_t* cond) { 
    pthread_cond_t* realCond = (pthread_cond_t*)getSyncEntry(cond);

    if(!global_isRollback()) {
      if(isInvalidSyncVar(realCond)) {
        cond_init((pthread_cond_t*)cond, NULL);
        realCond = (pthread_cond_t*)getSyncEntry(cond);
      }
			return Real::pthread_cond_broadcast(realCond); 
		}
		else {
			// During the rollback pahse, this should be NULL. 
			assert(realCond != NULL);
			return 0;
		}
	}

  int cond_signal(pthread_cond_t* cond) { 
    pthread_cond_t* realCond = (pthread_cond_t*)getSyncEntry(cond);

    if(!global_isRollback()) {
      if(isInvalidSyncVar(realCond)) {
        cond_init((pthread_cond_t*)cond, NULL);
        realCond = (pthread_cond_t*)getSyncEntry(cond);
      }
			return Real::pthread_cond_signal(realCond); 
		}
		else {
			// During the rollback pahse, this should be NULL. 
			assert(realCond != NULL);
			return 0;
		}
	}

  // Barrier support
  int barrier_init(pthread_barrier_t* barrier, const pthread_barrierattr_t* attr, unsigned int count) {
    int result = 0;
    
		pthread_barrier_t* realBarrier = NULL;

    if(!global_isRollback()) {
      // Allocate a real mutex.
      realBarrier = (pthread_barrier_t*)allocSyncEntry(sizeof(pthread_barrier_t), E_SYNC_BARRIER);

      // Actually initialize this mutex
      result = Real::pthread_barrier_init(realBarrier, attr, count);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(E_SYNCVAR_BARRIER, barrier, realBarrier, sizeof(pthread_barrier_t));
    }
    return result;
  }

  int barrier_destroy(pthread_barrier_t* barrier) {
    deferSync((void*)barrier, E_SYNCVAR_BARRIER);
    return 0;
  }

  ///// mutex functions

  // Add the barrier support.
  int barrier_wait(pthread_barrier_t* barrier) {
    int ret;
    pthread_barrier_t* realBarrier = NULL;
    SyncEventList* list = NULL;

    realBarrier = (pthread_barrier_t*)getSyncEntry(barrier);
    assert(realBarrier != NULL);
    list = getSyncEventList(barrier, sizeof(pthread_barrier_t));

    if(!global_isRollback()) {
      // Since we do not have a lock here, which can not guarantee that
      // the first threads cross this will be the first ones pass
      // actual barrier. So we only record the order to pass the barrier here.
      ret = Real::pthread_barrier_wait(realBarrier);
      list->recordSyncEvent(E_SYNC_BARRIER, ret);
    } else {
      ret = _sync.peekSyncEvent(list);
      if(ret == 0) {
        waitSemaphore();
      }

			// FIXME: how to 
      updateSyncEvent(list);

      if(ret == 0) {
        ret = Real::pthread_barrier_wait(realBarrier);
      }
    }

    return ret;
  }

  /*
    // Support for sigwait() functions in order to avoid deadlock.
    int sig_wait(const sigset_t *set, int *sig) {
      int ret;
      waitToken();
      epochEnd(false);

      ret = determ::getInstance().sig_wait(set, sig, _thread_index);
      if(ret == 0) {
        epochBegin(true);
      }

      return ret;
    }
  */

  /*
    // Now we need to save the context
    inline static void saveContext() {
      size_t size;

      PRINF("SAVECONTEXT: Current %p current->privateTop %p at %p thread index %d\n", current,
    current->privateTop, &current->privateTop, current->index);
      // Save the stack at first.
      current->privateStart = &size;
      size = size_t((intptr_t)current->privateTop - (intptr_t)current->privateStart);
      current->backupSize = size;

      if(size >= current->totalPrivateSize) {
        PRINF("Wrong. Current stack size (%lx = %p - %p) need to backup is larger than" \
                "total size (%lx). Either the application called setrlimit or the implementation" \
                "is wrong.\n", size, current->privateTop, current->privateStart,
    current->totalPrivateSize);
        Real::exit(-1);
      }

     // PRINF("privateStart %p size %lx backup %p\n", current->privateStart, size, current->backup);
      memcpy(current->backup, current->privateStart, size);
      // We are trying to save context at first
      getcontext(&current->context);
    }
  */

  // Save the given signal handler.
  void saveSpecifiedContext(ucontext_t* context) {
    current->newContext.saveSpecifiedContext(context);
  }

  // Return actual thread index
  int getIndex() { return current->index; }

  // Preparing the rollback.
  void prepareRollback();
	void prepareRollbackAlivethreads();
	void destroyAllSemaphores();
	void initThreadSemaphore(thread_t* thread);
	void destroyThreadSemaphore(thread_t* thread);
	void wakeupOldWaitingThreads();

  static void epochBegin();

  // Rollback the current thread
  static void rollbackCurrent();
	void checkRollbackCurrent();

	// It is called when a thread has to rollback.
	// Thus, it will replace the current context (of signal handler)
	// with the old context.
  void rollbackInsideSignalHandler(ucontext* context) {
    xcontext::rollbackInsideSignalHandler(context, &current->oldContext);
  }

  inline static pthread_t thread_self() { return Real::pthread_self(); }

  inline static void saveContext() {
    current->oldContext.saveContext();
  };

  inline static void restoreContext() {
    PRINF("restore context now\n");
    xcontext::restoreContext(&current->oldContext, &current->newContext);
  };

  // Now we will change the newContext to old context.
  // And also, we will restore context based on newContext.
  inline static void resetContexts() {
    xcontext::resetContexts(&current->oldContext, &current->newContext);

    // Reset the context to the new context for this thread.
    // setcontext (&_context);
    //    memcpy(&current->oldContext, &current->newContext, sizeof(xcontext));
    assert(0);
  }

  // We will rollback based on old context. We will leave newContext intactly
  inline static void rollbackContext() { assert(0); }

  // Run those deferred synchronization.
  inline static void runDeferredSyncs() { threadinfo::getInstance().runDeferredSyncs(); }

  //
  inline static bool hasReapableThreads() { return threadinfo::getInstance().hasReapableThreads(); }

  inline static void enableCheck() {
    current->internalheap = false;
    current->disablecheck = false;
    // PRINF("Enable check for current %p disablecheck %d\n", current, current->disablecheck);
  }

  inline static bool isCheckDisabled() { return current->disablecheck; }

  inline static void disableCheck() {
    current->internalheap = true;
    current->disablecheck = true;
    // PRINF("Disable check for current %p disablecheck %d\n", current, current->disablecheck);
  }

  inline static pid_t gettid() { return syscall(SYS_gettid); }

  static void invokeCommit();
  bool addQuarantineList(void* ptr, size_t sz);
  static bool isThreadSafe(thread_t * thread);

private:
  inline void* getSyncEntry(void* entry) {
    void** ptr = (void**)entry;
    return (*ptr);
  }

  inline SyncEventList* getSyncEventList(void* ptr, size_t size) {
    void** entry = (void**)ptr;
    //    PRINF("ptr %p *entry is %p, size %ld\n", ptr, *entry, size);
    return (SyncEventList*)((intptr_t)(*entry) + size);
  }

  inline void setSyncEntry(syncVariableType type, void* syncvar, void* realvar, size_t size) {
    unsigned long* target = (unsigned long*)syncvar;
    unsigned long expected = *(unsigned long*)target;

    // if(!__sync_bool_compare_and_swap(target, expected, (unsigned long)realvar)) {
    if(!__atomic_compare_exchange_n(target, &expected, (unsigned long)realvar, false,
                                    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
      deallocSyncEntry(realvar);
    } else {
      // It is always safe to add this list into corresponding map since
      // only those passed the CAS instruction can call this.
      // Thus, it is not existing two lists corresponding to the same synchronization variable.
      SyncEventList* list = getSyncEventList(syncvar, size);

      // Adding this entry to global synchronization map
      _sync.insertSyncMap(type, (void*)syncvar, realvar, list);
    }
  }

  inline void* allocSyncEntry(int size, thrSyncCmd synccmd) {
    // We allocate a synchorniation event list here and attach to this real
    // synchronziation variable so that they can be deleted in the same time.
    void* entry = ((void*)InternalHeap::getInstance().malloc(size + sizeof(SyncEventList)));
    assert(entry != NULL);
    void* ptr = (void*)((intptr_t)entry + size);

    // Using placement new here
    new (ptr) SyncEventList(entry, synccmd);
    return entry;
  }

  inline void deallocSyncEntry(void* ptr) { InternalHeap::getInstance().free(ptr); }

  // Acquire the semaphore for myself.
  // If it is my turn, I should get the semaphore.
  static void waitSemaphore() {
    semaphore* sema = &current->sema;

		PRINF("wait on semaphore %p\n", sema);
    sema->get();
		PRINF("Get the semaphore %p\n", sema);
  }

  semaphore* getSemaphore() { return &current->sema; }

  // Newly created thread should call this.
  inline static void threadRegister(bool isMainThread) {
    pid_t tid = gettid();
    void* privateTop;
    size_t stackSize = __max_stack_size;

    current->self = Real::pthread_self();

    // Initialize event pool for this thread.
    listInit(&current->pendingSyncevents);

    // Lock the mutex for this thread.
    lock_thread(current);

    // Initialize corresponding cond and mutex.
    //listInit(&current->list);

    current->tid = tid;
    current->status = E_THREAD_RUNNING;
    current->isNewlySpawned = true;

    current->disablecheck = false;

    // FIXME: problem
    current->joiner = NULL;

    // Initially, we should set to check system calls.
    enableCheck();

    // Initialize the localized synchronization sequence number.
    // pthread_t thread = current->self;
    pthread_t thread = pthread_self();

    if(isMainThread) {
      void* stackBottom;
      current->mainThread = true;

      // First, we must get the stack corresponding information.
      selfmap::getInstance().getStackInformation(&stackBottom, &privateTop);
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
      current->mainThread = false;
      // Calculate the top of this page.
      privateTop = (void*)(((intptr_t)thread + xdefines::PageSize) & ~xdefines::PAGE_SIZE_MASK);
    }

    current->oldContext.setupStackInfo(privateTop, stackSize);
    current->newContext.setupStackInfo(privateTop, stackSize);
    current->stackTop = privateTop;
    current->stackBottom = (void*)((intptr_t)privateTop - stackSize);

    // Now we can wakeup the parent since the parent must wait for the registe
    signal_thread(current);

    PRINF("THREAD%d (pthread_t %p) registered at %p, status %d wakeup %p. lock at %p\n",
          current->index, (void*)current->self, current, current->status, &current->cond,
          &current->mutex);

    unlock_thread(current);
    if(!isMainThread) {
      // Save the context for non-main thread.
      saveContext();
    }

    // WARN("THREAD%d (pthread_t %p) registered at %p", current->index, current->self, current );
    PRINF("THREAD%d (pthread_t %p) registered at %p, status %d\n", current->index,
          (void*)current->self, current, current->status);
  }

  static bool isThreadDetached() { return current->isDetached; }

  void cleanSyncEvents() { _sync.cleanSyncEvents(); }

  /// @ internal function: allocation a thread index
  int allocThreadIndex() { return _thread.allocThreadIndex(); }

  inline thread_t* getThreadInfo(int index) { return _thread.getThreadInfo(index); }

  inline thread_t* getThread(pthread_t thread) {
    return threadmap::getInstance().getThreadInfo(thread);
  }

  // Actually calling both pdate both thread event list and sync event list.
  inline void updateSyncEvent(SyncEventList* list) {
		// Advance the thread eventlist
		_sync.advanceThreadSyncList();

    struct syncEvent* event = list->advanceSyncEvent();
    if(event) {
      // Actually, we will wakeup next thread on the event list.
      // Since cond_wait will call unlock at first.
      _sync.signalNextThread(event);
    }
  }

  inline void insertAliveThread(thread_t* thread, pthread_t tid) {
    threadmap::getInstance().insertAliveThread(thread, tid);
  }

  inline static void insertDeadThread(thread_t* thread) {
  }

  static bool isStackVariable(void* ptr) {
    return (ptr >= current->stackBottom && ptr <= current->stackTop);
  }

  // Insert a synchronization variable into the global list, which
  // are reaped later in the beginning of next epoch.
  inline static bool deferSync(void* ptr, syncVariableType type) {
    // We won't handle the re-use the same synchronization variable in the same epoch.
		// That is, one synchronization variable will have two different purposes, which are detroyed 
		// at first and then re-utilize for another purpose. 
    return threadinfo::getInstance().deferSync(ptr, type);
  }

  static void setThreadSafe();
  static void setThreadUnsafe();

  // @Global entry of all entry function.
  static void* startThread(void* arg) {
    void* result = NULL;
    current = (thread_t*)arg;

    // PRINF("thread %p self %p is starting now.\n", current, (void*)current->self);
    // Record some information before the thread moves on
    threadRegister(false);

    // Now current thread is safe to be interrupted.
    setThreadSafe();
    PRINF("thread %p self %p after thread register now.\n", current, (void*)current->self);

    PRINF("Child thread %d has been registered.\n", current->index);
    // We actually get those parameter about new created thread
    // from the TLS storage.
    result = current->startRoutine(current->startArg);
    PRINF("result %p of calling startRoutine %p on thread %d\n", result, current->startRoutine, current->index);
    // Insert dead list now so that the corresponding entry can be cleaned up if
    // there is no need to rollback.

    // Lock the mutex for this thread.
    lock_thread(current);

    current->result = result;
    current->status = E_THREAD_WAITFOR_REAPING;

    // Only check the joiner when the thread is not deatached.
    if(!isThreadDetached()) {
      // Check the state of joiner.
      if(current->joiner) {
        assert(current->joiner->status == E_THREAD_JOINING);
        PRINF("Waking up the joiner %p!!!\n", (void*)current->joiner->self);
        // Now we can wakeup the joiner.
        signal_thread(current);
      }
    } else {
      PRINF("Thread is detached!!!\n");
    }

    // At commit points, if no heap overflow is detected, then the thread
    // should set the status to E_THREAD_EXITING, otherwise it should
    // be set to E_THREAD_ROLLBACK
    while(current->status != E_THREAD_EXITING && current->status != E_THREAD_ROLLBACK) {
      wait_thread(current);
    }

    // What to do in the end of a thread?
		// It can only have two status, one is to rollback and another is to exit successfully.
    if(current->status == E_THREAD_ROLLBACK) {
      PRINF("THREAD%d (at %p) is wakenup and plan to rollback\n", current->index, current);
      unlock_thread(current);

      // Rollback: has the possible race conditions. FIXME
      // Since it is possible that we copy back everything after the join, then
      // Some thread data maybe overlapped by this rollback.
      // Especially those current->joiner, then we may have a wrong status.
      PRINF("THREAD%d (at %p) is rollngback now\n", current->index, current);
      xthread::getInstance().rollbackCurrent();

      // We will never reach here
      assert(0);
    } else {
      assert(current->status == E_THREAD_EXITING);
      PRINF("THREAD%d (at %p) is wakenup and plan to exit now\n", current->index, current);
      unlock_thread(current);
    }
    return result;
  }

  // They are claimed in xthread.cpp since I don't
  // want to create an xthread.cpp
  xsync _sync;
	SysRecord _sysrecord;
  threadinfo& _thread;
  SyncEventList* _spawningList;
};

#endif
