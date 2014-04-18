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
 * @file   threadstruct.h
 * @brief  Definition of thread related structure. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _THREADSTRUCT_H_
#define _THREADSTRUCT_H_

#include <ucontext.h>
#include "xcontext.h"
#include "semaphore.h"
#include "recordentries.h"
#include "quarantine.h"

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

typedef struct thread {
  bool      available; // True: the thread index is free.
  bool      internalheap;
  // Should we disable check or not?
  bool      disablecheck; 
 // bool      isSpawning; // Whether a new thread is spawning?  
  bool      isNewlySpawned;  // whether this thread is spawned in this epoch?
  // Whether this thread has been joined or not. 
  // If the thread has not been joined, then we can't reap this thread.
  // Otherwise, pthread_join may crash since the thread has exited/released.
  bool      hasJoined;
  bool      isSafe; // whether a thread is safe to be interrupted 
  bool      waitSafe; // whether a thread is safe to be interrupted 
  int       index;
  pid_t     tid; // Current process id of this thread.
  pthread_t self; // Results of pthread_self
  thrStatus status;

  // We will use this to link this thread to other lists. 
  list_t   list;

  // We have to allocate the space for all record initially.
  void *   record;
    
  // mutex when a thread is trying to change its state.
  // In fact, this mutex is only protect joiner.
  // Only in the beginning of a thread (register),
  // we need to care about the joiner 
  pthread_mutex_t mutex; 
  pthread_cond_t  cond;

  // if a thread is detached, then the current thread don't need to wait its parent
  bool isDetached;    

  // Local output buffer for each thread. In order to avoid malloc requests in the 
  // replaying.
  char  outputBuf[LOG_SIZE];

  // What is the parent of this thread 
  struct thread * parent;

  struct thread * joiner;

  // Synchronization events happens on this thread.
  RecordEntries<struct syncEvent> syncevents;

  quarantine qlist;

 // struct syncEventList syncevents;
  list_t pendingSyncevents;
 // struct syncEventList pendingSyncevents;

  // We used this to record the stack range
  void * stackBottom;
  void * stackTop;

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

#endif
