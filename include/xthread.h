// -*- C++ -*-

/*
 Copyright (c) 2007-2013 , University of Massachusetts Amherst.

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
 * @file   xthread.h
 * @brief  Handling different kinds of synchronizations, like thread creation and exit, 
 *         lock, conditional variables and barriers.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XTHREAD_H_
#define _XTHREAD_H_

#include <sys/types.h>
#include <syscall.h>
#include <sys/syscall.h>
#include "xdefines.h"
//#include "atomic.h"
//#include "xmemory.h"
#include "real.h"
#include "threadinfo.h"
#include "globalinfo.h"
//#include "syscalls.h"
#include "threadmap.h"
#include "record.h"
#include "synceventlist.h"
#include "internalsyncs.h"
#include "list.h"

class xthread {

public:
  xthread()
  : _thread(threadinfo::getInstance())
  {
  }

  // Actually, it is not an actual singleton. 
  // Every process will have one copy. They can be used 
  // to hold different contents specific to different threads.
  static xthread& getInstance() {
    static char buf[sizeof(xthread)]; 
    static xthread * xthreadObject = new(buf) xthread();
    return * xthreadObject;
  }

  void initialize() {
    //DEBUG("%p: THREADD initialize nnnnnnnnnnnnnnnnnnnn\n", current);
    _thread.initialize();

    // Initialize the syncmap and threadmap.
    _sync.initialize();
    threadmap::getInstance().initialize();
 
    DEBUG("Starting thread initialization");

    // Initialize the global list for spawning operations 
    void * ptr = ((void *)InternalHeap::getInstance().malloc(sizeof(SyncEventList)));
    _spawningList = new (ptr) SyncEventList(NULL, E_SYNC_SPAWN);
   
    // We do not know whether NULL can be support or not, so we use
    // fake variable name _spawningList here 
    _sync.insertSyncMap((void *)_spawningList, _spawningList, _spawningList); 

    // Register the first thread 
    initialThreadRegister();
    current->isSafe = true;
    DEBUG("Done with thread initialization"); 
  }

  void finalize() {
    threadmap::getInstance().finalize();
  }

  // After an epoch is end and there is no overflow,
  // we should discard those record events since there is no
  // need to rollback anymore 
  // tnrere are three types of events here.
  void epochEndWell() {
  //  runDeferredSyncs();

    // The global syncrecord
    cleanSyncEvents();
  }

  // Register initial thread
  inline void initialThreadRegister() {
    int tindex = allocThreadIndex();
 
    thread_t * tinfo = getThreadInfo(tindex);

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
  int thread_exit(void * value) {
    // FIXME later.
    abort();
  }

  // In order to improve the speed, those spawning operations will do in 
  // a batched way. Everything else will be stopped except this spawning process.
  // All newly spawned children will also wait for the notification of the parent.
  // SO only the first time, the thread will wait on fence.
  // To guarantee the correctness, those newly spawned threads will issue 
  // an epochBegin() to discard those possibly polluted pages.
  // For the parent, because no one is running when spawnning, so there is no
  // need to call epochBegin(). 
  int thread_create(pthread_t * tid, const pthread_attr_t * attr, threadFunction * fn, void * arg) {
    void * ptr = NULL;
    int tindex;
    int result;

//   DEBUG("****in the beginning of thread_create, *tid is %lx\n", *tid);
    if(!global_isRollback()) {
//      DEBUG("PTHREAD_CREATE it is not rollback phase!!!!!!\n");
      // Lock and record
      global_lock();

      // Allocate a global thread index for current thread.
      tindex = allocThreadIndex();

      // This can be caused by two reasons:
      // First, xdefines::MAX_ALIVE_THREADS is too small.
      // Second, we haven't meet commit point for a long time.
      if(tindex == -1) {
        REQUIRE(hasReapableThreads(), "No reapable threads");
        global_unlock();
        
        invokeCommit();
 
        global_lock();
        tindex = allocThreadIndex();
        REQUIRE(tindex != -1, "System can support %d threads", xdefines::MAX_ALIVE_THREADS);
        DEBUG("AFTER commit now******* tindex %d\n", tindex);     
      }


      DEBUG("thread creation with index %d\n", tindex);
      // WRAP up the actual thread function.
      // Get corresponding thread_t structure.
      thread_t * children = getThreadInfo(tindex);
     
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
      

      // Now we are going to record this spawning event.
      setThreadSpawning();
      result =  Real::pthread_create()(tid, attr, xthread::startThread, (void *)children);
      unsetThreadSpawning();
      if(result != 0) {
        DEBUG("thread creation failed with errno %d -- %s\n", errno, strerror(errno));
        Real::exit()(-1);
      }
    
      // Record spawning event
      _spawningList->recordSyncEvent(E_SYNC_SPAWN, result);
      getRecord()->recordCloneOps(result, *tid);

      if(result == 0) {
        insertAliveThread(children, *tid);
      }

      global_unlock();
      
      DEBUG("Creating thread %d at %p self %p\n", tindex, children, (void*)children->self); 
      if(result == 0) {
        // Waiting for the finish of registering children thread.
        lock_thread(children);

        while(children->status == E_THREAD_STARTING) {
          wait_thread(children);
    //     DEBUG("Children %d status %d. now wakenup\n", children->index, children->status);
        }
        unlock_thread(children);
      }
    }
    else {
      DEBUG("process %d is before thread_create now\n", current->index);
      result = _sync.peekSyncEvent();

      getRecord()->getCloneOps(tid, &result);
      DEBUG("process %d in creation, result %d\n", current->index, result);
      if(result == 0) { 
        waitSemaphore();
        DEBUG("process %d is after waitsemaphore\n", current->index);

        // Wakeup correponding thread, now they can move on.  
        thread_t * thread = getThread(*tid);
        DEBUG("Waken up *tid %p thread %p child %d in thread_creation\n", (void*)*tid, thread, thread->index);
         
        // Wakeup corresponding thread
        thread->joiner = NULL;
        thread->status = E_THREAD_ROLLBACK;
        signal_thread(thread);
      }
      // Whenever we are calling __clone, then we can ask the thread to rollback?
      // Update the events.
     DEBUG("#############process %d before updateSyncEvent now\n", current->index);
      updateSyncEvent(_spawningList); 
//      _spawningList->advanceSyncEvent(); 
    }

    return result;
  }

  /// @brief Wait for a thread to exit.
  /// Should we record this? It is not necessary?? FIXME
  inline int thread_join(pthread_t joinee, void ** result) {
    thread_t * thread = NULL;

   // DEBUG("thread_join on joinee %p\n", joinee);    
    // Try to check whether thread is empty or not? 
    thread = getThread(joinee);
    assert(thread != NULL);

    //WARN("thread_join, joinee is 0x%lx thread %p thread->index %d*****\n", joinee, thread, thread->index); 
//    DEBUG("thread_join, joinee is 0x%lx thread %p thread->status %d*****\n", joinee, thread, thread->status); 

    // Now the thread has finished the register
    lock_thread(thread);
   
    if(thread->status != E_THREAD_WAITFOR_REAPING) {
      //WARN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
      // Set the joiner to current thread 
      thread->joiner = current;
      current->status = E_THREAD_JOINING;
  
//      WARN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
      // Now we are waiting the finish of child thread   
      while(current->status != E_THREAD_RUNNING) {
      //  WARN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
      //  DEBUG("thread_join, current %p status %d, waiting on joinee %d (at %p, thread %p). thread->joiner %p at %p*****\n", current, current->status, thread->index, thread, thread->self, thread->joiner, &thread->joiner); 
        // Wait for the joinee to wake me up
        wait_thread(thread);
      //  DEBUG("thread_join status %d, wakenup by thread %d*****\n", current->status, thread->index); 
      }
      
     // WARN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
    } 

    // Now mark this thread's status so that the thread can be reaped.
    thread->hasJoined = true;

    // FIXME: actually, we should get the result from corresponding thread
    if(result) {
     // DEBUG("thread_join, getresult, result %p actual result*****\n", result); 
      *result = thread->result;
    }

    // Now we unlock and proceed
    unlock_thread(thread);
    
    insertDeadThread(thread);    

    return 0;
  }


  /// @brief Detach a thread
  inline int thread_detach(pthread_t thread) {
    thread_t * threadinfo = NULL;
   
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
    retval= Real::pthread_cancel()(thread);
    if(retval == 0) {
      threadinfo::getInstance().cancelAliveThread(thread);
    }
    return retval; // EDB
  }

  inline int thread_kill(pthread_t thread, int sig) {
    return Real::pthread_kill()(thread, sig);
  }

  inline void setRealSyncVariable(void * syncVar, void * realVar) {
  }

  inline bool setSyncEventList(void * syncVar, void * syncList) {
    return true; // EDB FIX ME
  }

  /// Save those actual mutex address in original mutex.
  int mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t *attr) {
    pthread_mutex_t * realMutex = NULL;
    int result = 0;

    if(!global_isRollback()) {
      // Allocate a real mutex.
      realMutex=(pthread_mutex_t *)allocSyncEntry(sizeof(pthread_mutex_t), E_SYNC_MUTEX_LOCK);

//      DEBUG("mutex_init with realMutex %p\n", realMutex);
      // Actually initialize this mutex
      result = Real::pthread_mutex_init()(realMutex, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(mutex, realMutex, sizeof(pthread_mutex_t));
    }
    return result;
  }


  int do_mutex_lock(void * mutex, thrSyncCmd synccmd) {
    int ret;
    pthread_mutex_t * realMutex = NULL;
    SyncEventList * list = NULL;

    //DEBUG("do_mutex_lock\n"); 
    if(!global_isRollback()) {
      //DEBUG("do_mutex_lock before getSyncEntry %d\n", __LINE__); 
      realMutex = (pthread_mutex_t *)getSyncEntry(mutex);
     // DEBUG("do_mutex_lock after getSyncEntry %d realMutex %p\n", __LINE__, realMutex); 
      if(realMutex == NULL) {
        mutex_init((pthread_mutex_t *)mutex, NULL);
        realMutex = (pthread_mutex_t *)getSyncEntry(mutex);
     //   DEBUG("do_mutex_lock after getSyncEntry %d realMutex %p\n", __LINE__, realMutex); 
      }
      
      assert(realMutex != NULL);

      switch(synccmd) {
        case E_SYNC_MUTEX_LOCK:
          ret = Real::pthread_mutex_lock()(realMutex);
          break;

        case E_SYNC_MUTEX_TRY_LOCK:
          ret = Real::pthread_mutex_trylock() (realMutex);
          break;

        default:
          break;
      }

      // Record this event
    //  DEBUG("do_mutex_lock before recording\n"); 
      list = getSyncEventList(mutex, sizeof(pthread_mutex_t)); 
      list->recordSyncEvent(E_SYNC_MUTEX_LOCK, ret);
    }
    else {
      list = getSyncEventList(mutex, sizeof(pthread_mutex_t));
     // DEBUG("synceventlist get mutex at %p list %p\n", mutex, list);
      assert(list != NULL);
      ret = _sync.peekSyncEvent();
      if(ret == 0) { 
        waitSemaphore();
      }

      // Update thread synchronization event in order to handle the nesting lock.
      _sync.advanceThreadSyncList();
    }
    //DEBUG("do_mutex_lock done!!!!!\n"); 
    return ret;
  }
  
  int mutex_lock(pthread_mutex_t * mutex) {
   return do_mutex_lock(mutex, E_SYNC_MUTEX_LOCK);
  }
  
  int mutex_trylock(pthread_mutex_t * mutex) {
   return do_mutex_lock(mutex, E_SYNC_MUTEX_TRY_LOCK);
  }

  int mutex_unlock(pthread_mutex_t * mutex) {
    int ret = 0;
    pthread_mutex_t * realMutex = NULL;

    if(!global_isRollback()) {
      realMutex = (pthread_mutex_t *)getSyncEntry(mutex);
      ret = Real::pthread_mutex_unlock()(realMutex);
    }
    else {
      SyncEventList * list = getSyncEventList(mutex, sizeof(pthread_mutex_t)); 
      struct syncEvent * nextEvent = list->advanceSyncEvent();
      if(nextEvent) {
        _sync.signalNextThread(nextEvent);
      }
    }
   // WARN("mutex_unlock mutex %p\n", mutex);
    return ret;
  }

  // Add this event into the destory list.
  int mutex_destroy(pthread_mutex_t * mutex) {
    deferSync((void *)mutex, E_SYNCVAR_MUTEX);
    return 0; // EDB
  }
  
  ///// conditional variable functions.
  void cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr) {
    Real::pthread_cond_init()(cond, attr);
  }

  // Add this into destoyed list.
  void cond_destroy(pthread_cond_t *  cond) {
    deferSync((void *)cond, E_SYNCVAR_COND);
  }

  // Condwait: since we usually get the mutex before this. So there is 
  // no need to check mutex any more.
  int cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
    int ret; 
    SyncEventList * list = getSyncEventList(mutex, sizeof(pthread_mutex_t)); 
    assert(list != NULL);

    if(!global_isRollback()) {
      pthread_mutex_t * realMutex = (pthread_mutex_t *)getSyncEntry(mutex);
      assert(realMutex != NULL);

      DEBUG("cond_wait for thread %d\n", current->index);
      // Add the event into eventlist
      ret = Real::pthread_cond_wait() (cond, realMutex);
      
      // Record the waking up of conditional variable
      list->recordSyncEvent(E_SYNC_MUTEX_LOCK, ret);
    }
    else {
      ret = _sync.peekSyncEvent();
      
      if(ret == 0) {
        struct syncEvent * event = list->advanceSyncEvent();
        if(event) {
          // Actually, we will wakeup next thread on the event list.
          // Since cond_wait will call unlock at first.
          _sync.signalNextThread(event);
        }

        // Now waiting for the lock
        waitSemaphore();
      }

      _sync.advanceThreadSyncList();
    }

    return ret;
  }
  
  int cond_broadcast(pthread_cond_t * cond) {
    return Real::pthread_cond_broadcast()(cond);
  }

  int cond_signal(pthread_cond_t * cond) {
    return Real::pthread_cond_signal()(cond);
  }
  
  // Barrier support
  int barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t * attr, unsigned int count) {
    int result = 0;
#ifndef BARRIER_SUPPORT
    // Look for this barrier in the map of initialized barrieres.
    result = Real::pthread_barrier_init()(barrier, attr, count);
#else
    pthread_barrier_t * realBarrier = NULL;

    if(!global_isRollback()) {
      // Allocate a real mutex.
      realBarrier=(pthread_barrier_t *)allocSyncEntry(sizeof(pthread_barrier_t), E_SYNC_BARRIER);

      // Actually initialize this mutex
      result = Real::pthread_barrier_init()(realBarrier, attr);

      // If we can't setup this entry, that means that this variable has been initialized.
      setSyncEntry(barrier, realBarrier, sizeof(pthread_barrier_t));
    }
#endif
    return result;
  }

  int barrier_destroy(pthread_barrier_t *barrier) {
    deferSync((void *)barrier, E_SYNCVAR_BARRIER);
    return 0;
  }

  ///// mutex functions

  // Add the barrier support.
  int barrier_wait(pthread_barrier_t *barrier) {
    int ret;
#ifndef BARRIER_SUPPORT
    ret = Real::pthread_barrier_wait()(barrier);
#else
    pthread_barrier_t * realBarrier = NULL;
    SyncEventList * list = NULL;

    realBarrier = (pthread_barrier_t *)getSyncEntry(barrier);
    assert(realBarrier != NULL);
    list = getSyncEventList(var, sizeof(pthread_barrier_t)); 

    if(!global_isRollback()) {
      // Since we do not have a lock here, which can not guarantee that
      // the first threads cross this will be the first ones pass
      // actual barrier. So we only record the order to pass the barrier here.
      ret = Real::pthread_barrier_wait()(realBarrier);
      list->recordSyncEvent(E_SYNC_BARRIER, ret);
    }
    else {
      ret = _sync.peekSyncEvent();
      if(ret == 0) {
        waitSemaphore();
      }

      updateSyncEvent(list);

      if(ret == 0) {
        ret = Real::pthread_barrier_wait()(realBarrier);
      }
    }
#endif
      
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

    DEBUG("SAVECONTEXT: Current %p current->privateTop %p at %p thread index %d\n", current, current->privateTop, &current->privateTop, current->index);
    // Save the stack at first.
    current->privateStart = &size;
    size = size_t((intptr_t)current->privateTop - (intptr_t)current->privateStart);
    current->backupSize = size;

    if(size >= current->totalPrivateSize) {
      DEBUG("Wrong. Current stack size (%lx = %p - %p) need to backup is larger than" \
              "total size (%lx). Either the application called setrlimit or the implementation" \
              "is wrong.\n", size, current->privateTop, current->privateStart, current->totalPrivateSize);
      Real::exit()(-1);
    }
 
   // DEBUG("privateStart %p size %lx backup %p\n", current->privateStart, size, current->backup);
    memcpy(current->backup, current->privateStart, size);
    // We are trying to save context at first
    getcontext(&current->context);
  }
*/
  
  // Save the given signal handler.
  void saveSpecifiedContext(ucontext_t * context) {
    current->newContext.saveSpecifiedContext(context);
  }

  // Return actual thread index  
  int getIndex() {
    return current->index;
  }

  // Preparing the rollback.
  void prepareRollback();

  static void epochBegin();

  // Rollback the whole process
  static void rollback();
 
  void rollbackInsideSignalHandler(ucontext * context) {
    
    xcontext::rollbackInsideSignalHandler(context, &current->oldContext);
 //   restoreContext(context);
  }

  inline static pthread_t thread_self() {
    return Real::pthread_self()();
  }

  inline static void saveContext() {
    current->oldContext.saveContext();
  };

  inline static void restoreContext() {
    DEBUG("restore context now\n");
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
  inline static void rollbackContext() {
   assert(0); 
  }

  // Run those deferred synchronization.
  inline static void runDeferredSyncs() {
    threadinfo::getInstance().runDeferredSyncs();
  }

  // 
  inline static bool hasReapableThreads() {
    return threadinfo::getInstance().hasReapableThreads();
  }
  
  inline bool threadSpawning() {
    return current->isSpawning;
  }


  void invokeCommit();
  bool addQuarantineList(void * ptr, size_t sz);

private:
  inline void * getSyncEntry(void * entry) {
    void ** ptr = (void **)entry;
    return(*ptr);
  }

  inline SyncEventList * getSyncEventList(void * ptr, size_t size) {
    void ** entry = (void **)ptr;
    //DEBUG("ptr %p *entry is %p, size %d\n", ptr, *entry, size); 
    return (SyncEventList *)((intptr_t)(*entry) + size);
  }

  inline void setSyncEntry(void * syncvar, void * realvar, size_t size) {
    if(!atomic_compare_and_swap((unsigned long *)syncvar, 0, (unsigned long)realvar)) {
      deallocSyncEntry(realvar);
    }
    else {
      // It is always safe to add this list into corresponding map since
      // only those passed the CAS instruction can call this.
      // Thus, it is not existing two lists corresponding to the same synchronization variable.
      SyncEventList * list = getSyncEventList(syncvar, size);

      // Adding this entry to global synchronization map
      _sync.insertSyncMap((void *)syncvar, realvar, list); 
    }
  }

  inline void * allocSyncEntry(int size, thrSyncCmd synccmd) {
    SyncEventList * syncList;
    
    // We allocate a synchorniation event list here and attach to this real 
    // synchronziation variable so that they can be deleted in the same time.
    void * entry = ((void *)InternalHeap::getInstance().malloc(size + sizeof(SyncEventList)));
    assert(entry != NULL);
    void * ptr = (void *)((intptr_t)entry + size);

    // Using placement new here
    syncList = new (ptr) SyncEventList(entry, synccmd);
    return entry;
  }

  inline void deallocSyncEntry(void *ptr) {
    InternalHeap::getInstance().free(ptr);
  } 

  
  static Record * getRecord() {
    return (Record *)current->record;
  }

  // Acquire the semaphore for myself.
  // If it is my turn, I should get the semaphore.
  static void waitSemaphore() {
    semaphore * sema = &current->sema;
    sema->get();
  }

  semaphore * getSemaphore() {
    return &current->sema;
  }

  inline static void setThreadSpawning() {
    current->internalheap = true;
    current->isSpawning = true;
  }

  inline static void unsetThreadSpawning() {
    current->internalheap = false;
    current->isSpawning = false;
  }
  
  inline static pid_t gettid() {
    return syscall(SYS_gettid);
  }
    
  // Newly created thread should call this.
  inline static void threadRegister(bool isMainThread) {
    pid_t pid = syscall(SYS_getpid);
    pid_t tid = gettid();
    void * privateTop;
    size_t stackSize = __max_stack_size;

    current->self = Real::pthread_self()();

    // Initialize event pool for this thread.
    current->syncevents.initialize(xdefines::MAX_SYNCEVENT_ENTRIES);
    listInit(&current->pendingSyncevents);

    // Lock the mutex for this thread.
    lock_thread(current);
    
    // Initialize corresponding cond and mutex.
    listInit(&current->list);
 
    current->tid = tid;
    current->status = E_THREAD_RUNNING;
    current->isNewlySpawned = true;
  
    // FIXME: problem
    current->joiner = NULL;

    // Initially, we should set to check system calls.
    unsetThreadSpawning();

    // Initialize the localized synchronization sequence number.
    //pthread_t thread = current->self;
    pthread_t thread = pthread_self();

    if(isMainThread) {
      void * stackBottom;
      current->mainThread = true;

      // First, we must get the stack corresponding information.
      selfmap::getInstance().getStackInformation(&stackBottom, &privateTop);
    }
    else {
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
      privateTop = (void *)(((intptr_t)thread + xdefines::PageSize) & ~xdefines::PAGE_SIZE_MASK);
    }  
   
    current->oldContext.setupStackInfo(privateTop, stackSize);
    current->newContext.setupStackInfo(privateTop, stackSize);
    current->stackTop = privateTop;
    current->stackBottom = (void *)((intptr_t)privateTop - stackSize);
      
    // Now we can wakeup the parent since the parent must wait for the registe
    signal_thread(current);

    DEBUG("THREAD%d (pthread_t %p) registered at %p, status %d wakeup %p. lock at %p\n", current->index, (void*)current->self, current, current->status, &current->cond, &current->mutex);
   
    unlock_thread(current); 
    if(!isMainThread) {
      // Save the context for non-main thread.
      saveContext();
    }

    //WARN("THREAD%d (pthread_t %p) registered at %p", current->index, current->self, current );
    DEBUG("THREAD%d (pthread_t %p) registered at %p, status %d\n", current->index, (void*)current->self, current, current->status);
  }


  static bool isThreadDetached() {
    return current->isDetached;
  }

  void cleanSyncEvents() {
    _sync.cleanSyncEvents();
  }

  /// @ internal function: allocation a thread index
  int allocThreadIndex() {
    return _thread.allocThreadIndex();
  }
  
  inline thread_t * getThreadInfo(int index) {
    return _thread.getThreadInfo(index);
  }
  
  inline thread_t * getThread(pthread_t thread) {
    return threadmap::getInstance().getThreadInfo(thread);
  }

  // Actually calling both pdate both thread event list and sync event list.
  inline void updateSyncEvent(SyncEventList * list) {
    // First step, advance thread's list
    _sync.advanceThreadSyncList();

    struct syncEvent * event = list->advanceSyncEvent();
    if(event) {
      // Actually, we will wakeup next thread on the event list.
      // Since cond_wait will call unlock at first.
      _sync.signalNextThread(event);
    }
  }

  inline void insertAliveThread(thread_t * thread, pthread_t tid) {
    threadmap::getInstance().insertAliveThread(thread, tid);
  }

  inline static void insertDeadThread(thread_t * thread) {
    threadinfo::getInstance().insertDeadThread(thread);
  }

  static bool isStackVariable(void * ptr) {
    return(ptr >= current->stackBottom && ptr <= current->stackTop);
  }

  // Insert a synchronization variable into the global list, which 
  // are reaped later at commit points.
  inline static void deferSync(void * ptr, syncVariableType type) {
    // Checking whether the ptr is globals or heap variables
    // we should not record ptr for stack variable since it will change
    if(!isStackVariable(ptr)) {
      threadinfo::getInstance().deferSync(ptr, type);
    }
  }

  static void setThreadSafe();

  // @Global entry of all entry function.
  static void * startThread(void * arg) {
    void * result;
    current = (thread_t *)arg;

    DEBUG("thread %p self %p is starting now.\n", current, (void*)current->self);
    // Record some information before the thread moves on
    threadRegister(false);
    
    // Now current thread is safe to be interrupted. 
    setThreadSafe(); 
    DEBUG("thread %p self %p after thread register now.\n", current, (void*)current->self);
 
    DEBUG("Child thread %d has been registered.\n", current->index);
    // We actually get those parameter about new created thread
    // from the TLS storage.
    result = current->startRoutine(current->startArg);

    // Insert dead list now so that the corresponding entry can be cleaned up if
    // there is no need to rollback.

    // Lock the mutex for this thread.
    lock_thread(current);
    
    current->result = result;

    // Only check the joiner when the thread is not deatached.
    if(!isThreadDetached()) {
      thread_t * joiner;
      // Check the state of joinning thread.
      joiner = current->joiner; 

      // Check the state of joiner.
      if(joiner) {
        assert(joiner->status == E_THREAD_JOINING);
        joiner->status = E_THREAD_RUNNING;
        DEBUG("Waking up the joiner %p!!!\n", (void*)joiner->self);
        // Now we can wakeup the joiner.
        signal_thread(current);
      }
    } else {
      DEBUG("Thread is detached!!!\n");
    } 

    current->status = E_THREAD_WAITFOR_REAPING;

    // At commit points, if no heap overflow is detected, then the thread
    // should set the status to E_THREAD_EXITING, otherwise it should 
    // be set to E_THREAD_ROLLBACK 
    //while(current->status != E_THREAD_EXITING && current->status != E_THREAD_ROLLBACK) {     
    while(current->status == E_THREAD_WAITFOR_REAPING) {
      DEBUG("DEAD thread %d is sleeping, status is %d\n", current->index, current->status);
      DEBUG("DEAD thread %d is sleeping, status is %d\n", current->index, current->status);
      wait_thread(current);
//      DEBUG("DEAD thread %d is wakenup status is %d\n", current->index, current->status);
    }

    // What to do in the end of a thread? Who will send out this message? 
    // When heap overflow occurs 
    if(current->status == E_THREAD_ROLLBACK) {
      DEBUG("THREAD%d (at %p) is wakenup\n", current->index, current);
      current->status = E_THREAD_RUNNING;
      unlock_thread(current);

      // Rollback: has the possible race conditions. FIXME
      // Since it is possible that we copy back everything after the join, then
      // Some thread data maybe overlapped by this rollback.
      // Especially those current->joiner, then we may have a wrong status.
      DEBUG("THREAD%d (at %p) is rollngback now\n", current->index, current);
      xthread::getInstance().rollback();

      // We will never reach here
      assert(0);
    }
    else {
      assert(current->status == E_THREAD_EXITING);
      current->syncevents.finalize();
      DEBUG("THREAD%d (at %p) is exiting now\n", current->index, current);
      unlock_thread(current);
    }
    return result; // EDB
  }
  
  // They are claimed in xthread.cpp since I don't
  // want to create an xthread.cpp
  xsync _sync;
  threadinfo & _thread;
  SyncEventList * _spawningList;
};

#endif
