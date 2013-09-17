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
 * @file   threadmap.h
 * @brief  Mapping between pthread_t and internal thread information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _THREADMAP_H_
#define _THREADMAP_H_

#include <sys/types.h>
#include <syscall.h>
#include <sys/syscall.h>
#include "xdefines.h"
#include "hashmap.h"
#include "hashfuncs.h"
#include "spinlock.h"
#include "xcontext.h"
#include "semaphore.h"
#include "record.h"

extern "C" {
  typedef enum e_thrstatus {
    E_THREAD_STARTING = 0,
    E_THREAD_RUNNING,
    E_THREAD_JOINING, // The thread is trying to join other threads.
    E_THREAD_EXITING, // The thread is exiting.
//    E_THREAD_EXITED, // The thread is exiting.
  //  E_THREAD_SIGNALED, // The thread has been signaled, waiting for the instruction
  //  E_THREAD_CONTINUE, // The thread should move forward.
    E_THREAD_ROLLBACK,
    E_THREAD_WAITFOR_JOINING, // The thread has finished and wait for the joining.
  
    // Thread are not exiting to guarantee the reproducibility
    // It marks its status E_THREAD_WAITFOR_REAPING, one thread 
    // entering the committing phase should reap all wa 
    E_THREAD_WAITFOR_REAPING, 
  } thrStatus;
  
  typedef enum e_thrsynccmd {
    E_SYNC_SPAWN = 0,  // Thread creation
  //  E_SYNC_COND_SIGNAL,// conditional signal
  //  E_SYNC_COND_BROADCAST,// conditional broadcast
  //  E_SYNC_COND_WAIT,// conditional wait
  //  E_SYNC_COND_WAKEUP,// conditional wakeup from waiting
    E_SYNC_BARRIER, // barrier waiting 
    E_SYNC_LOCK, // Inside the critical section. 
    E_SYNC_THREAD, // Not a actual synchronization event, but for each thread.
  //  E_SYNC_KILL, // Inside the critical section. 
  } thrSyncCmd;
  
  // When a thread is trying to joining a children,
  // it will setup joiner information for the children it try to wait for.
  // Each synchronization variable will have a corresponding structure.
  struct syncEventList {
    list_t list;
    pthread_mutex_t lock;
    void     * syncVariable; 
    thrSyncCmd syncCmd;
    list_t   * curentry; // Used to traverse all entries of this list.
  };

#if 0
  // System calls results when a thread is spawning
  struct threadCreation {
    // Normally, two mmaps will be called when thread creation.
    bool   hasCallMmaped; // which mmap are called?
    void * mmapAddr1;
    void * mmapAddr2;
    pid_t  tid;
  };
#endif

  typedef struct thread {
    bool      available; // True: the thread index is free.
    bool      isSpawning; // Whether a new thread is spawning?  
    bool      isNewlySpawned;  // whether this thread is spawned in this epoch?
    // Whether this thread has been joined or not. 
    // If the thread has not been joined, then we can't reap this thread.
    // Otherwise, pthread_join may crash since the thread has exited/released.
    bool      hasJoined;
    int       isSafe; // whether a thread is safe to be interrupted 
    int       index;
    pid_t     tid; // Current process id of this thread.
    pthread_t self; // Results of pthread_self
    thrStatus status;

    // We will use this to link this thread to other lists. 
    list_t   list;

    // We have to allocate the space for all record initially.
    // Sorry, we can't use the class record here since it is "C" not "C++"
    void *   record;    
    // mutex when a thread is trying to change its state.
    // In fact, this mutex is only protect joiner.
    // Only in the beginning of a thread (register),
    // we need to care about the joiner 
    pthread_mutex_t mutex; 
    pthread_cond_t  cond;

    // if a thread is detached, then the current thread don't need to wait its parent
    bool isDetached;    
 
    // What is the parent of this thread 
    struct thread * parent;

    struct thread * joiner;

    // Synchronization events happens on this thread.
    struct syncEventList syncevents;
    struct syncEventList pendingSyncevents;

    // We used this to record the stack range
    void * stackBottom;
    void * stackTop;

    // The local thread output buffer.
    char  outputBuf[LOG_SIZE];

    // Main thread have completely stack setting.
    bool mainThread;

    semaphore sema; 
    // We need to keep two context: one is old context, which is saved in the beginning of
    // transaction. another one is new context, which is normally saved in the signal handler.
    // For example, if one thread is going to commit, it is going to signal other threads to stop.
    // so we need another context to save context.
    xcontext oldContext;
    xcontext newContext;

    // The following is the parameter about starting function. 
    threadFunction * startRoutine;
    void * startArg;
    void * result;
  } thread_t;

  // The following structure will be added to alivelist 
  struct aliveThread {
    list_t list;
    thread_t * thread;
  };

  // Record corresponding information for each event.  
  struct syncEvent {
    list_t     list;
    list_t     threadlist; // Adding myself to the threadeventlist

    // Which thread is performing synchronization? 
    thread_t*  thread;
    struct syncEventList * eventlist;
    int        ret; // used for mutex_lock
  };

  // A pending synchronization event needed to be handled by corresponding
  // thread.  
  struct pendingSyncEvent {
    list_t list;
    struct syncEvent * event;
  };

  // Each thread has corresponding status information in a global array.

  // We will maintain an array about the status of each thread.
  // Actually, there are two status that will be handled by us.
  extern __thread thread_t * current;
};

class threadmap {

public:
  threadmap()
  {
  }
 
  static threadmap& getInstance (void) {
    static char buf[sizeof(threadmap)];
    static threadmap * theOneTrueObject = new (buf) threadmap();
    return *theOneTrueObject;
  }
 
  void initialize() {
//    fprintf(stderr, "xmap initializeNNNNNNNNNNNNNN\n");
    _xmap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::THREAD_MAP_SIZE);
    
    listInit(&_alivethreads);
  } 

  // Destroy all semaphores
  void finalize() {
    //fprintf(stderr, "Destroy all semaphores NOOOOOOOOO!\n");
    destroyAllSemaphores();
  }

  thread_t * getThreadInfo(pthread_t thread) {
    thread_t * info = NULL;
    _xmap.find((void *)thread, sizeof(void *), &info);
    //PRERR("getThreadInfo now, thread info %p\n", info);
    return info;
  } 
 
  void deleteThreadMap(pthread_t thread) {
    _xmap.erase((void *)thread, sizeof(void*));
  }

  void insertAliveThread(thread_t * thread, pthread_t tid) {
    // Malloc 
    struct aliveThread * ath = (struct aliveThread *)InternalHeap::getInstance().malloc(sizeof(struct aliveThread));

    listInit(&ath->list);
    ath->thread = thread;

    //PRDBG("Insert alive thread %lx\n", thread);
    listInsertTail(&ath->list, &_alivethreads);

    _xmap.insert((void *)tid, sizeof(void *), thread);
  }

  void removeAliveThread(thread_t * thread) {
    // First, remove thread from the threadmap.
    deleteThreadMap(thread->self);

    // Second, remove thread from the alive list.
    struct aliveThread * ath;

    // Now the _alivethreads list should not be empty.
    assert(isListEmpty(&_alivethreads) != true);
   
    // Search the whole list for given tid.
    ath = (struct aliveThread *)nextEntry(&_alivethreads);
    while(true) {
      //PRDBG("Traverse thread %lx\n", ath->thread);
      // If we found the entry, remove this entry from the list.
      if(ath->thread == thread) {
        listRemoveNode(&ath->list);
        break;
      }
      // It is impossible that we haven't find the node until the end of a list.
      if(isListTail(&ath->list, &_alivethreads) == true) {
        PRDBG("WRong, we can't find alive thread %lx (thread %p)in the list.\n", thread, thread);
        EXIT;
      }
      
      ath = (struct aliveThread *)nextEntry(&ath->list);
    }

    InternalHeap::getInstance().free((void *)ath);

    // Setting a thread structure to be "Free" status.
    setFreeThread(thread);
  }
  
  // Set a threadInfo structure to be free.
  void setFreeThread(thread_t * thread) {
    thread->available = true;
  }
 
  // How to return a thread event from specified entry.
  inline struct syncEvent * getThreadEvent(list_t * entry) {
    // FIXME: if we changed the structure of syncEvent.
    int threadEventOffset = sizeof(list_t);

    return(struct syncEvent *)((intptr_t)entry - threadEventOffset);
  }
 
  /*
   prepareRollback:
    each thread will update its synchronization entry to the first one on its synchronization list.
   */
  void prepareRollback(void) {
    struct aliveThread * ath;
    thread_t * thread;
    struct syncEventList * eventlist;
 
    // Search the whole list for given tid.
    ath = (struct aliveThread *)nextEntry(&_alivethreads);
    while(true) {
      thread_t * thread = ath->thread;

      // Initialize the semaphore for this thread.
      initThreadSemaphore(thread);
       
      // Set the entry of each thread to the first synchronization event.
      eventlist = &thread->syncevents;
      if(!isListEmpty(&eventlist->list)) {
        // Pointing to first synchronization of this thread
        eventlist->curentry = nextEntry(&eventlist->list);
      }

      // Update to the next thread.
      if(isListTail(&ath->list, &_alivethreads)) {
        break;
      }
      else {
        ath = (struct aliveThread *)nextEntry(&ath->list);
      }
    }
  }

  /*
    destroy all semaphores:
   */
  void destroyAllSemaphores(void) {
    struct aliveThread * ath;
    thread_t * thread;
    struct syncEventList * eventlist;
 
    // Search the whole list for given tid.
    ath = (struct aliveThread *)nextEntry(&_alivethreads);
    while(true) {
      thread_t * thread = ath->thread;

      // If we found the entry, remove this entry from the list.
      destroyThreadSemaphore(thread);
  
      // Update to the next thread.
      if(isListTail(&ath->list, &_alivethreads)) {
        break;
      }
      else {
        ath = (struct aliveThread *)nextEntry(&ath->list);
      }
    }
  }

  // Initialize the semaphore for  specified thread
  void destroyThreadSemaphore(thread_t * thread) {
    semaphore * sema = &thread->sema;

    // We initialize the semaphore value to 0.
    sema->destroy();
  }

  // Initialize the semaphore for  specified thread
  void initThreadSemaphore(thread_t * thread) {
    semaphore * sema = &thread->sema;

    PRDBG("INITSEMA: THREAD%d at %p sema %p\n", thread->index, thread, sema);
    // We initialize the semaphore value to 0.
    sema->init((unsigned long)thread->self, 1, 0);
  }

public:  
  class aliveThreadIterator
  {
    struct aliveThread * thread;

    public:
    aliveThreadIterator(struct aliveThread * ithread = NULL)
    {
      thread = ithread;
    }

    ~aliveThreadIterator()
    {
    }
    
    aliveThreadIterator& operator++(int unused)// in postfix ++
    {
      if(!isListTail(&thread->list, &_alivethreads)) {
        thread = (struct aliveThread *)nextEntry(&thread->list); 
      }
      else {
        thread = NULL;
      }

      return *this;
    }

    //aliveThreadIterator& operator -- ();
    // Iterpreted as a = b is treated as a.operator=(b)
    aliveThreadIterator& operator=(const aliveThreadIterator & that) {
      thread = that.thread;
      return *this;
    } 

    bool operator==(const aliveThreadIterator& that) const
    { return thread == that.thread; }

    bool operator != (const aliveThreadIterator& that) const
    { return thread != that.thread; }

    thread_t * getThread() {
      return thread->thread; 
    }
  };

  void traverseAllThreads() {
    struct aliveThread * ath;
    thread_t * thread;

 
    // Search the whole list for given tid.
    ath = (struct aliveThread *)nextEntry(&_alivethreads);
    while(true) {
      thread_t * thread = ath->thread;

      if(thread->status != E_THREAD_WAITFOR_REAPING) {
      fprintf(stderr, "thread %p self %p status %d\n", thread, thread->self, thread->status);
      }

      // Update to the next thread.
      if(isListTail(&ath->list, &_alivethreads)) {
        break;
      }
      else {
        ath = (struct aliveThread *)nextEntry(&ath->list);
      }
    }
  }


  // Acquire the first entry of the hash table
  aliveThreadIterator begin() {
    // Get the first non-null entry
    if(isListEmpty(&_alivethreads) != true) {
      return aliveThreadIterator((struct aliveThread *)nextEntry(&_alivethreads));
    }
    else {
      return end();
    }
  }

  aliveThreadIterator end() {
    return aliveThreadIterator(NULL);
  }

private:
  // We are maintainning a private hash map for each thread.
  typedef HashMap<void *, thread_t *, spinlock, InternalHeapAllocator> threadHashMap;

  // The  variables map shared by all threads
  threadHashMap _xmap;

  static list_t _alivethreads;
};

#endif
