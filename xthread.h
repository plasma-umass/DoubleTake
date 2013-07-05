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
#include "libfuncs.h"
#include "threadinfo.h"
#include "globalinfo.h"
//#include "syscalls.h"
#include "record.h"
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
  static xthread& getInstance (void) {
    static char buf[sizeof(xthread)]; 
    static xthread * xthreadObject = new(buf) xthread();
    return * xthreadObject;
  }

  void initialize(void) {
    _thread.initialize();
 
    PRWRN("threadinit"); 
    // Register the first thread 
    initialThreadRegister();
    current->isSafe = true;
    PRWRN("threadinit after"); 
  }

  void finalize(void) {
    _thread.finalize(); 
  }

  // After an epoch is end and there is no overflow,
  // we should discard those record events since there is no
  // need to rollback anymore 
  // tnrere are three types of events here.
  void epochEndWell(void) {
  //  runDeferredSyncs();

    // The global syncrecord
    cleanSyncEvents();
  }

  // Register initial thread
  inline void initialThreadRegister(void) {
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
    assert(0);
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

    globalinfo::getInstance().setMultithreading();

//   PRDBG("****in the beginning of thread_create, *tid is %lx\n", *tid);
    if(!isRollback()) {
   //fprintf(stderr, "****in the beginning of thread_create, *tid is %lx\n", *tid);
      // Lock and record
      global_lock();

      // Allocate a global thread index for current thread.
      tindex = allocThreadIndex();

      // This can be caused by two reasons:
      // First, xdefines::MAX_ALIVE_THREADS is too small.
      // Second, we haven't meet commit point for a long time.
      if(tindex == -1) {
        if(hasReapableThreads() == false) {
          PRERR("xdefines::MAX_ALIVE_THREADS is set to too small\n");
          EXIT;
        }
        global_unlock();
        
        invokeCommit();
 
        global_lock();
        tindex = allocThreadIndex();
        if(tindex == -1) {
          PRDBG("System can support %d simultaneously: xdefines::MAX_ALIVE_THREADS to a larger number\n", xdefines::MAX_ALIVE_THREADS);
          EXIT;
        }
        PRDBG("AFTER commit now******* tindex %d\n", tindex);     
      }


      //PRDBG("thread creation with index %d\n", tindex);
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

//      PRDBG("*********%d: before creating a thread\n", getpid());
      // Now we are going to record this spawning event.
      setThreadSpawning();
      result =  WRAP(pthread_create)(tid, attr, xthread::startThread, (void *)children);
      unsetThreadSpawning();
      if(result != 0) {
        PRDBG("thread creation failed with errno %d -- %s\n", errno, strerror(errno));
        WRAP(exit)(-1);
      }
    
      // Record spawning event
      recordSyncEvent(NULL, E_SYNC_SPAWN, result);
      getRecord()->recordCloneOps(result, *tid);

      if(result == 0) {
        insertAliveThread(children, *tid);
      }

      global_unlock();
      
      //PRDBG("Creating THREAD%d at %p self %p\n", tindex, children, children->self); 
      if(result == 0) {
        // Waiting for the finish of registering children thread.
        WRAP(pthread_mutex_lock)(&children->mutex);
        while(children->status == E_THREAD_STARTING) {
      //    PRDBG("Children %d (at %p) status %d waiting at %p. Holding lock %p\n", children->index, *tid, children->status, &children->cond, &children->mutex);
          WRAP(pthread_cond_wait)(&children->cond, &children->mutex);
    //     PRDBG("Children %d status %d. now wakenup\n", children->index, children->status);
        }
        WRAP(pthread_mutex_unlock)(&children->mutex);
      }
    }
    else {
      PRDBG("process %d is before thread_create now\n", current->index);
      result = peekSyncEvent(NULL, E_SYNC_SPAWN);
      getRecord()->getCloneOps(tid, &result);
      PRWRN("process %d in creation, result %d\n", current->index, result);
      if(result == 0) { 
        waitSemaphore();
        PRWRN("process %d is after waitsemaphore\n", current->index, result);

        // Wakeup correponding thread, now they can move on.  
        thread_t * thread = getThread(*tid);
        PRWRN("Waken up *tid %p thread %p child %d in thread_creation\n", *tid, thread, thread->index);
         
        // Wakeup corresponding thread
        thread->joiner = NULL;
        thread->status = E_THREAD_ROLLBACK;
        WRAP(pthread_cond_signal)(&thread->cond);
      }
      // Whenever we are calling __clone, then we can ask the thread to rollback?
      // Update the events.
     PRDBG("#############process %d before updateSyncEvent now\n", current->index); 
      updateSyncEvent(NULL, E_SYNC_SPAWN); 
    }

    return result;
  }

  /// @brief Wait for a thread to exit.
  /// Should we record this? It is not necessary?? FIXME
  inline int thread_join(pthread_t joinee, void ** result) {
    thread_t * thread = NULL;

   // PRDBG("thread_join on joinee %p\n", joinee);    
    // Try to check whether thread is empty or not? 
    thread = getThread(joinee);
    assert(thread != NULL);

    //PRWRN("thread_join, joinee is 0x%lx thread %p thread->index %d*****\n", joinee, thread, thread->index); 
//    PRDBG("thread_join, joinee is 0x%lx thread %p thread->status %d*****\n", joinee, thread, thread->status); 

    // Now the thread has finished the register
    WRAP(pthread_mutex_lock)(&thread->mutex);
   
    if(thread->status != E_THREAD_WAITFOR_REAPING) {
      //PRWRN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
      // Set the joiner to current thread 
      thread->joiner = current;
      current->status = E_THREAD_JOINING;
  
//      PRWRN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
      // Now we are waiting the finish of child thread   
      while(current->status != E_THREAD_RUNNING) {
      //  PRWRN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
      //  PRDBG("thread_join, current %p status %d, waiting on joinee %d (at %p, thread %p). thread->joiner %p at %p*****\n", current, current->status, thread->index, thread, thread->self, thread->joiner, &thread->joiner); 
        // Wait for the joinee to wake me up
        WRAP(pthread_cond_wait)(&thread->cond, &thread->mutex);
      //  PRDBG("thread_join status %d, wakenup by thread %d*****\n", current->status, thread->index); 
      }
      
     // PRWRN("thread_join, thread->index %d status %d*****\n", thread->index, thread->status); 
    } 

    // Now mark this thread's status so that the thread can be reaped.
    thread->hasJoined = true;

    // FIXME: actually, we should get the result from corresponding thread
    if(result) {
     // PRDBG("thread_join, getresult, result %p actual result*****\n", result); 
      *result = thread->result;
    }

    // Now we unlock and proceed
    WRAP(pthread_mutex_unlock)(&thread->mutex);
    
    insertDeadThread(thread);    

    return 0;
  }


  /// @brief Detach a thread
  inline int thread_detach(pthread_t thread) {
    thread_t * threadinfo = NULL;
   
    // Try to check whether thread is empty or not? 
    threadinfo = getThreadInfo(thread);

    assert(threadinfo != NULL);

    WRAP(pthread_mutex_lock)(&threadinfo->mutex);
    threadinfo->isDetached = true;
    WRAP(pthread_mutex_unlock)(&threadinfo->mutex);
    
    assert(0);
  }
  
  /// @brief Do a pthread_cancel
  inline int thread_cancel(pthread_t thread) {
    int retval;
    invokeCommit();
    retval= WRAP(pthread_cancel)(thread);
    if(retval == 0) {
      threadinfo::getInstance().cancelAliveThread(thread);
    }
  }

  inline int thread_kill(pthread_t thread, int sig) {
    return WRAP(pthread_kill)(thread, sig);
  }

  /// Save those actual mutex address in original mutex.
  int mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t *attr) {
    WRAP(pthread_mutex_init)(mutex, attr);
    if(!isRollback()) {
      allocSyncEventList((void *)mutex, E_SYNC_LOCK);
    }
    return 0;
  }

  int mutex_lock(pthread_mutex_t * mutex) {
    int ret;
 
    if(globalinfo::getInstance().isMultithreading()) {
      if(!isRollback()) {
        ret = WRAP(pthread_mutex_lock) (mutex);

        // Record this event 
        recordSyncEvent((void *)mutex, E_SYNC_LOCK, ret);
      }
      else {
        ret = peekSyncEvent(mutex, E_SYNC_LOCK);
        if(ret == 0) { 
          waitSemaphore();
        }

        // Update thread synchronization event in order to handle the nesting lock.
        updateThreadSyncList(mutex);
      }
    }
    else {
      ret = WRAP(pthread_mutex_lock) (mutex);
    }
    return ret;
  }
  
  int mutex_trylock(pthread_mutex_t * mutex) {
    int ret;

    if(globalinfo::getInstance().isMultithreading()) {
      if(!isRollback()) {
        ret = WRAP(pthread_mutex_trylock) (mutex);

        // Record this event 
        recordSyncEvent((void *)mutex, E_SYNC_LOCK, ret);
      }
      else {
        ret = peekSyncEvent(mutex, E_SYNC_LOCK);
        if(ret == 0) { 
          waitSemaphore();
        }

        // Update thread synchronization event in order to handle the nesting lock.
        updateThreadSyncList(mutex);
      }
    }
    else {
      ret = WRAP(pthread_mutex_trylock) (mutex);
    }

    return ret;
  }

  int mutex_unlock(pthread_mutex_t * mutex) {
    int ret = 0;

    if(globalinfo::getInstance().isMultithreading()) {
      if(!isRollback()) {
        ret = WRAP(pthread_mutex_unlock) (mutex);
      }
      else {
  //    PRWRN("Update the sync event on lock (UNLOCK)\n");
        updateMutexSyncList(mutex); 
      //updateSyncEvent(mutex, E_SYNC_LOCK); 
      }
    }
    else {
      ret = WRAP(pthread_mutex_unlock) (mutex);
    }
    return ret;
  }

  // Add this event into the destory list.
  int mutex_destroy(pthread_mutex_t * mutex) {
    deferSync((void *)mutex, E_SYNCVAR_MUTEX);
  }
  
  ///// conditional variable functions.
  void cond_init(pthread_cond_t * cond, const pthread_condattr_t * attr) {
    WRAP(pthread_cond_init)(cond, attr);
    //allocSyncEventList((void *)cond);
  }

  // Add this into destoyed list.
  void cond_destroy(pthread_cond_t *  cond) {
    deferSync((void *)cond, E_SYNCVAR_COND);
  }

  // Condwait: since we usually get the mutex before this. So there is 
  // no need to check mutex any more.
  int cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex) {
    int ret; 

    if(!isRollback()) {
      //PRDBG("cond_wait for thread %d\n", current->index);
      // Add the event into eventlist
      ret = WRAP(pthread_cond_wait) (cond, mutex);
      PRDBG("cond_wait wakendup for thread %d, another mutex_lock\n", current->index);
      recordSyncEvent((void *)mutex, E_SYNC_LOCK, ret);
    }
    else {
      // Actually, we will wakeup next thread on the event list.
      // Since cond_wait will call unlock at first.
      ret = peekSyncEvent((void *)mutex, E_SYNC_LOCK);
      updateMutexSyncList(mutex);

      // Then we will try to get semaphore in order to proceed.
      // It tried to acquire mutex_lock when waken up.
      if(ret == 0) {
        waitSemaphore();
      }

      updateThreadSyncList(mutex);
    }

    return ret;
  }
  
  int cond_broadcast(pthread_cond_t * cond) {
    return WRAP(pthread_cond_broadcast)(cond);
  }

  int cond_signal(pthread_cond_t * cond) {
    return WRAP(pthread_cond_signal)(cond);
  }
  
  // Barrier support
  int barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t * attr, unsigned int count) {
    // Look for this barrier in the map of initialized barrieres.
    int result = WRAP(pthread_barrier_init)(barrier, attr, count);

    if(!isRollback()) {
      allocSyncEventList((void *)barrier, E_SYNC_BARRIER);
    }
    return 0;
  }

  int barrier_destroy(pthread_barrier_t *barrier) {
    deferSync((void *)barrier, E_SYNCVAR_BARRIER);
    //PRDBG("Destory varrier barrier %p\n", barrier);
 //   WRAP(pthread_barrier_destroy)(barrier);
    return 0;
  }

  ///// mutex functions

  // Add the barrier support.
  int barrier_wait(pthread_barrier_t *barrier) {
    int ret;
#ifdef NO_BARRIER_SUPPORT
    ret = WRAP(pthread_barrier_wait)(barrier);
#else
    if(!isRollback()) {
      // Since we do not have a lock here, which can not guarantee that
      // the first threads cross this will be the first ones pass
      // actual barrier. So we only record the order to pass the barrier here.
      ret = WRAP(pthread_barrier_wait)(barrier);
      recordSyncEvent((void *)barrier, E_SYNC_BARRIER, ret);
    }
    else {
      ret = peekSyncEvent((void *)barrier, E_SYNC_BARRIER);

      if(ret == 0) {
        waitSemaphore();
      }

      //checkSyncEvent(barrier, E_SYNC_BARRIER); 
      updateSyncEvent(barrier, E_SYNC_BARRIER);
      if(ret == 0) {
        ret = WRAP(pthread_barrier_wait)(barrier);
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

    PRDBG("SAVECONTEXT: Current %p current->privateTop %p at %p thread index %d\n", current, current->privateTop, &current->privateTop, current->index);
    // Save the stack at first.
    current->privateStart = &size;
    size = size_t((intptr_t)current->privateTop - (intptr_t)current->privateStart);
    current->backupSize = size;

    if(size >= current->totalPrivateSize) {
      PRDBG("Wrong. Current stack size (%lx = %p - %p) need to backup is larger than" \
              "total size (%lx). Either the application called setrlimit or the implementation" \
              "is wrong.\n", size, current->privateTop, current->privateStart, current->totalPrivateSize);
      WRAP(exit)(-1);
    }
 
   // PRDBG("privateStart %p size %lx backup %p\n", current->privateStart, size, current->backup);
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
  int getIndex(void) {
    return current->index;
  }

  // Preparing the rollback.
  void prepareRollback(void);

  // Rollback the whole process
  inline void rollback(void) {
    // Recover the context for current thread.
    restoreContext(); 
  }
 
  void rollbackInsideSignalHandler(ucontext * context) {
    
    xcontext::rollbackInsideSignalHandler(context, &current->oldContext);
 //   restoreContext(context);
  }

  inline static pthread_t thread_self() {
    return WRAP(pthread_self)();
  }

  
  // The global lock
  static void global_lock(void) {
    globalinfo::getInstance().lock();
  }

  static void global_unlock(void) {
    globalinfo::getInstance().unlock();
  }

  inline static void saveContext() {
    current->oldContext.saveContext();
  };

  inline static void restoreContext() {
    PRDBG("restore context now\n");
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

  // Is this thread spawning?
  inline static bool threadSpawning() {
    return current->isSpawning == true;
  }
  
  // Run those deferred synchronization.
  inline static void runDeferredSyncs() {
    threadinfo::getInstance().runDeferredSyncs();
  }

  // 
  inline static bool hasReapableThreads() {
    return threadinfo::getInstance().hasReapableThreads();
  }

  void invokeCommit(void);

private:
  static Record * getRecord() {
    return (Record *)current->record;
  }

  // Acquire the semaphore for myself.
  // If it is my turn, I should get the semaphore.
  static void waitSemaphore() {
    semaphore * sema = &current->sema;
    sema->get();
  }

  semaphore * getSemaphore(void) {
    return &current->sema;
  }

  inline static void setThreadSpawning() {
    current->isSpawning = true;
  }

  inline static void unsetThreadSpawning() {
    current->isSpawning = false;
  }
  

  inline static pid_t gettid() {
    return syscall(SYS_gettid);
  }
    
  static void initThreadSyncEventList(struct syncEventList * eventlist) {
    listInit(&eventlist->list);
    eventlist->syncCmd = E_SYNC_THREAD;
    eventlist->syncVariable = current; // Thread
    WRAP(pthread_mutex_init)(&eventlist->lock, NULL); 
  }


  // Newly created thread should call this.
  inline static void threadRegister(bool isMainThread) {
    pid_t pid = syscall(SYS_getpid);
    pid_t tid = gettid();
    void * privateTop;
    size_t stackSize = __max_stack_size;

    current->self = WRAP(pthread_self)();

    // Lock the mutex for this thread.
    WRAP(pthread_mutex_lock)(&current->mutex);
    
    // Initialize corresponding cond and mutex.
    listInit(&current->list);
 
    current->tid = tid;
    current->status = E_THREAD_RUNNING;
    current->isNewlySpawned = true;
  
    // FIXME: problem
    current->joiner = NULL;

    initThreadSyncEventList(&current->syncevents);
    
    // Although this pendingSyncevents can be never used,
    // we still initialize here since it should not cost too much
    // because of limited amount of threads
    initThreadSyncEventList(&current->pendingSyncevents);

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
      
    // Now we can wakeup the parent since the parent must wait for the register
    WRAP(pthread_cond_signal)(&current->cond);

    PRDBG("THREAD%d (pthread_t %p) registered at %p, status %d wakeup %p. lock at %p\n", current->index, current->self, current, current->status, &current->cond, &current->mutex);
    
    WRAP(pthread_mutex_unlock)(&current->mutex);
    if(!isMainThread) {
      // Save the context for non-main thread.
      saveContext();
    }

    //PRWRN("THREAD%d (pthread_t %p) registered at %p", current->index, current->self, current );
    //PRDBG("THREAD%d (pthread_t %p) registered at %p, status %d\n", current->index, current->self, current, current->status);
  }


  static bool isThreadDetached(void) {
    return current->isDetached;
  }

  static bool isRollback() {
    return globalinfo::getInstance().isRollback();
  }

  void cleanSyncEvents() {
    _thread.cleanSyncEvents();
  }

  /// @ internal function: allocation a thread index
  int allocThreadIndex() {
    return _thread.allocThreadIndex();
  }
  
  inline thread_t * getThreadInfo(int index) {
    return _thread.getThreadInfo(index);
  }
  
  inline thread_t * getThread(pthread_t thread) {
    return _thread.getThread(thread);
  }

  // Optimization: do not need to record the synchronization event when there is only one thread
  // Mostly, we can save a lot of pthread calls.
  inline void recordSyncEvent(void * var, thrSyncCmd synccmd, int ret) {
    _thread.recordSyncEvent(var, synccmd, ret);
  }

  inline int peekSyncEvent(void * var, thrSyncCmd synccmd) {
    return _thread.peekSyncEvent(var, synccmd);
  }

  inline void allocSyncEventList(void * var, thrSyncCmd synccmd) {
    _thread.allocSyncEventList(var, synccmd);
  }

  inline void updateSyncEvent(void * var, thrSyncCmd synccmd) {
    _thread.updateSyncEvent(var, synccmd);
  }

  // Only mutex_lock can call this function. 
  inline void updateThreadSyncList(pthread_mutex_t *  mutex) {
    _thread.updateThreadSyncList(mutex);
  }
 
  inline void updateMutexSyncList(pthread_mutex_t * mutex) {
    _thread.updateMutexSyncList(mutex);
  }
 
  inline void insertAliveThread(thread_t * thread, pthread_t tid) {
    _thread.insertAliveThread(thread, tid);
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

  static void setThreadSafe(void);

  // @Global entry of all entry function.
  static void * startThread(void * arg) {
    void * result;
    current = (thread_t *)arg;

    //PRDBG("thread %p self %p is starting now.\n", current, current->self);
    // Record some information before the thread moves on
    threadRegister(false);
    
    // Now current thread is safe to be interrupted. 
    setThreadSafe(); 
    //PRDBG("thread %p self %p after thread register now.\n", current, current->self);
 
    //PRDBG("Child thread %d has been registered.\n", current->index);
    // We actually get those parameter about new created thread
    // from the TLS storage.
    result = current->startRoutine(current->startArg);

    // Insert dead list now so that the corresponding entry can be cleaned up if
    // there is no need to rollback.

    // Lock the mutex for this thread.
    WRAP(pthread_mutex_lock)(&current->mutex);
    
    current->result = result;

    // Only check the joiner when the thread is not deatached.
    if(!isThreadDetached()) {
      thread_t * joiner;
      // Check the state of joinning thread.
      joiner = current->joiner; 

#if 0
      if(joiner) {
        PRWRN("Thread %p (index %d) is not detachedi, joiner %p (THREAD%d) with status %d!!!\n", current, current->index, joiner->self, joiner->index, joiner->status);
      }
      else {
        PRWRN("Thread %p (index %d) is not detached, joiner %p is not existing!!!\n", current, current->index, joiner);
      }
#endif
      // Check the state of joiner.
      if(joiner) {
        assert(joiner->status == E_THREAD_JOINING);
        joiner->status = E_THREAD_RUNNING;
        PRWRN("Waking up the joiner %p!!!\n", joiner->self);
        // Now we can wakeup the joiner.
        WRAP(pthread_cond_signal)(&current->cond);
      }
    }
    else {
      PRDBG("Thread is detached!!!\n");
      PRWRN("Thread is detached!!!\n");
    } 

    current->status = E_THREAD_WAITFOR_REAPING;

    // At commit points, if no heap overflow is detected, then the thread
    // should set the status to E_THREAD_EXITING, otherwise it should 
    // be set to E_THREAD_ROLLBACK 
    //while(current->status != E_THREAD_EXITING && current->status != E_THREAD_ROLLBACK) {     
    while(current->status == E_THREAD_WAITFOR_REAPING) {
// && current->status != E_THREAD_ROLLBACK) {     
      //PRDBG("DEAD thread %d is sleeping, status is %d\n", current->index, current->status);
      //PRDBG("DEAD thread %d is sleeping, status is %d\n", current->index, current->status);
      WRAP(pthread_cond_wait)(&current->cond, &current->mutex);
//      PRDBG("DEAD thread %d is wakenup status is %d\n", current->index, current->status);
    }

    // What to do in the end of a thread? Who will send out this message? 
    // When heap overflow occurs 
    if(current->status == E_THREAD_ROLLBACK) {
      //PRDBG("THREAD%d (at %p) is wakenup\n", current->index, current);
      current->status = E_THREAD_RUNNING;
      WRAP(pthread_mutex_unlock)(&current->mutex);

      // Rollback: has the possible race conditions. FIXME
      // Since it is possible that we copy back everything after the join, then
      // Some thread data maybe overlapped by this rollback.
      // Especially those current->joiner, then we may have a wrong status.
      PRWRN("THREAD%d (at %p) is rollngback now\n", current->index, current);
      xthread::getInstance().rollback();

      // We will never reach here
      assert(0);
    }
    else {
      assert(current->status == E_THREAD_EXITING);
      PRWRN("THREAD%d (at %p) is exiting now\n", current->index, current);
      WRAP(pthread_mutex_unlock)(&current->mutex);
    }
  }
  
  // They are claimed in xthread.cpp since I don't
  // want to create an xthread.cpp
  threadinfo & _thread;
};

#endif
