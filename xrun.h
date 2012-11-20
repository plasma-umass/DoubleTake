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
 * @file   xrun.h
 * @brief  The main engine for consistency management, etc.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XRUN_H_
#define _XRUN_H_

#include "xdefines.h"

// threads
#include "xthread.h"

// memory
#include "xmemory.h"

// Heap Layers
#include "util/sassert.h"

#include "xsync.h"

// Stack 
#include "xcontext.h"

// Grace utilities
#include "atomic.h"

class xrun {

private:

  xrun (void)
  : _isInitialized (false),
    _isProtected (false),
    _hasRollbacked(false), 
    _memory (xmemory::getInstance())
  {
  //fprintf(stderr, "xrun constructor\n");
  }

public:

  static xrun& getInstance (void) {
    static char buf[sizeof(xrun)];
    static xrun * theOneTrueObject = new (buf) xrun();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize (void)
  {
    pid_t pid = syscall(SYS_getpid);
 
    if (!_isInitialized) {
      _isInitialized = true;

      // Initialize the xcontext.
      _context.initialize();
            
      // Initialize the memory (install the memory handler)
      _memory.initialize();

      // Set the current _tid to our process id.
      _thread.setId (pid);
      
      _tid = pid;
      _memory.setThreadIndex(0);

      // Save context. NOTE: only for the test
      saveContext();
      fprintf(stderr, "%d : initialize\n", getpid());
   } else {
      fprintf(stderr, "%d : OH NOES\n", getpid());
      ::abort();
      // We should only initialize ONCE.
    }
  }

  void openMemoryProtection(void) {
    if(_isProtected == false) {
      _memory.openProtection();
      _isProtected = true;
    }
  }

  void closeMemoryProtection(void) {
    _memory.closeProtection();
    _isProtected = false;
  }

  void finalize (void)
  {
#if 1
    if(_hasRollbacked == false) {
      // NOTE: only for the test
      atomicEnd();
     // rollback();
    }
#endif
    // If the tid was set, it means that this instance was
    // initialized: end the transaction (at the end of main()).
    _memory.finalize();
  }

  /* Transaction-related functions. */
  void saveContext(void) {
    // check whether we have opened the protection or not
    // If not, then we should do this before saving the context.
    if(_isProtected == false) {
      openMemoryProtection();
    }  

    _memory.atomicBegin();    

    // Now we may try to save context.
    _context.saveContext(); 
  }

  /// Rollback to previous 
  void rollback(void) {
    _hasRollbacked = true;

    fprintf(stderr, "\n\nNOW ROLLING BACK\n\n\n");
    
    // Rollback all memory before rolling back the context.
    _memory.rollback();

    _context.rollback();
  }
  
  /* Thread-related functions. */
  inline void threadRegister (void) {
    int threadindex;
      
    threadindex = atomic::increment_and_return(global_thread_index);
 
    // Since we are a new thread, we need to use the new heap.
    _memory.setThreadIndex(threadindex+1);

    //fprintf(stderr, "%d: threadindex %d, global_thread_index %d\n", getpid(), threadindex+1, *global_thread_index);
    return;
  }   

  inline void resetThreadIndex(void) {
   *global_thread_index = 0;
  }

  /// @ Return the main thread's id.
  inline int main_id(void) {
  return _tid;
  }

  /// @return the "thread" id.
  inline int id (void) const {
    return _thread.getId();
  }

  /// @brief Spawn a thread.
  /// @return an opaque object used by sync.
  inline void * spawn (threadFunction * fn, void * arg)
  {
    void * ptr = _thread.spawn (this, fn, arg);

    return ptr;
  }

  /// @brief Wait for a thread.
  inline void join (void * v, void ** result) {
    _thread.join (this, v, result);
  }

  /// @brief Do a pthread_cancel
  inline void cancel(void *v) {
    _thread.cancel(this, v);
  }

  inline void thread_kill(void *v, int sig) {
    atomicEnd();
    //_thread.thread_kill(this, v, sig);
    atomicBegin();
  } 

  /* Heap-related functions. */
  inline void * malloc (size_t sz) {
    void * ptr = _memory.malloc (sz);
    return ptr;
  }

  inline void * calloc (size_t nmemb, size_t sz) {
    void * ptr = malloc(nmemb * sz);
    return ptr;
  }

  // In fact, we can delay to open its information about heap.
  inline void free (void * ptr) {
    _memory.free (ptr);
  }

  inline size_t getSize (void * ptr) {
    return _memory.getSize (ptr);
  }

  inline void * realloc(void * ptr, size_t sz) {
    void * newptr;
    //fprintf(stderr, "realloc ptr %p sz %x\n", ptr, sz);
    if (ptr == NULL) {
      newptr = malloc(sz);
      return newptr;
    }
    if (sz == 0) {
      free (ptr);
      return NULL;
    }

    newptr = _memory.realloc (ptr, sz);
    //fprintf(stderr, "realloc ptr %p sz %x\n", newptr, sz);
    return newptr;
  }

  ///// conditional variable functions.
  void cond_init (void * cond) {
    _sync.cond_init(cond, false);
  }

  void cond_destroy (void * cond) {
    _sync.cond_destroy(cond);
  }

  // Barrier support
  int barrier_init(pthread_barrier_t  *barrier, unsigned int count) {
    return _sync.barrier_init(barrier, count);
  }

  int barrier_destroy(pthread_barrier_t *barrier) {
    _sync.barrier_destroy(barrier);
    return 0;
  }

  ///// mutex functions
  /// FIXME: maybe it is better to save those actual mutex address in original mutex.
  int mutex_init(pthread_mutex_t * mutex) {
    _sync.mutex_init(mutex, false);
    return 0;
  }

  // FIXME: if we are trying to remove atomicEnd() before mutex_sync(),
  // we should unlock() this lock if abort(), otherwise, it will
  // cause the dead-lock().
  void mutex_lock(pthread_mutex_t * mutex) {
//  fprintf(stderr, "%d : mutex lock\n", getpid());
    atomicEnd();
    _sync.mutex_lock(mutex);
    atomicBegin();
  }

  void mutex_unlock(pthread_mutex_t * mutex) {
    atomicEnd();
    _sync.mutex_unlock(mutex);
    atomicBegin();
  }

  int mutex_destroy(pthread_mutex_t * mutex) {
    return _sync.mutex_destroy(mutex);
  }


  int barrier_wait(pthread_barrier_t *barrier) {
    atomicEnd();
    _sync.barrier_wait(barrier);
    atomicBegin();
    return 0;
  }


  /// FIXME: whether we can using the order like this.
  void cond_wait(void * cond, void * lock) {
    atomicEnd();
    _sync.cond_wait (cond, lock);
    atomicBegin();
  }

  void cond_broadcast (void * cond) {
    atomicEnd();
    _sync.cond_broadcast (cond);
    atomicBegin();
  }

  void cond_signal (void * cond) {
    atomicEnd();
    _sync.cond_signal (cond);
    atomicBegin();
  }

  /// @brief Start a transaction.
  void atomicBegin (void) {
    if(!_isProtected)
      return;
    //fflush(stdout);

    // Now start.
    _memory.atomicBegin();
  }

  /// @brief End a transaction, aborting it if necessary.
  void atomicEnd (void) {
    if(!_isProtected)
      return;
 
    // First, attempt to commit.
    bool hasOverflow = _memory.atomicEnd();
    if(hasOverflow) {
      // Install watch point
      rollback();
    }

    // Flush the stdout.
    fflush(stdout);
  }

private:


  typedef enum { FAILED, SUCCEEDED } commitResult;


  xthread    _thread;

  xsync  _sync;

  /// The memory manager (for both heap and globals).
  xmemory&     _memory;

  xcontext _context;

  volatile  bool   _isInitialized;
  volatile  bool   _isProtected;
  volatile  bool   _hasRollbacked;
  int   _tid; //The first process's id.
};


#endif
