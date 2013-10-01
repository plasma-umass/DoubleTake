// -*- C++ -*-

/*
  Copyright (c) 2008-12, University of Massachusetts Amherst.

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
 * @file   threadinfo.h
 * @brief  Manageing the information about threads.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _THREADINFO_H_
#define _THREADINFO_H_

#include "xdefines.h"
#include "xmapping.h"
#include "threadmap.h"
#include "xsync.h"
#include "record.h"
#include "mm.h"

extern "C" {

  typedef enum e_syncVariable {
    E_SYNCVAR_THREAD = 0, 
    E_SYNCVAR_COND, 
    E_SYNCVAR_MUTEX, 
    E_SYNCVAR_BARRIER 
  } syncVariableType;

  // Each synchronization variable will have a corresponding list.
  struct deferSyncVariable {
    list_t list;
    syncVariableType syncVarType;
    void * variable;
  };
};

/// @class threadinfo
class threadinfo {
public:
  threadinfo()
  {

  }

  static threadinfo& getInstance (void) {
    static char buf[sizeof(threadinfo)];
    static threadinfo * theOneTrueObject = new (buf) threadinfo();
    return *theOneTrueObject;
  }

  void initialize(void) {
    _aliveThreads = 0;
    _reapableThreads = 0;
    _threadIndex = 0;
    
    _totalThreads = xdefines::MAX_ALIVE_THREADS;
  
    // Shared the threads information. 
    memset(&_threads, 0, sizeof(_threads));

    // Initialize the backup stacking information.
    size_t perStackSize = __max_stack_size;

    unsigned long totalStackSize = perStackSize * 2 * xdefines::MAX_ALIVE_THREADS;
    char * stackStart = (char *)MM::mmapAllocatePrivate(totalStackSize);

    // Initialize all mutex.
    thread_t * tinfo;
  
    for(int i = 0; i < xdefines::MAX_ALIVE_THREADS; i++) {
      tinfo = &_threads[i];
 
    //  PRWRN("in xthread initialize i %d\n", i ); 
      // Initialize the record.
      Record * sysrecord = (Record *)InternalHeap::getInstance().malloc(sizeof(Record));
    //  PRWRN("in xthread initialize i %d sysrecord %p\n", i , sysrecord); 
      sysrecord->initialize();
      tinfo->record = (void *)sysrecord;
    
      tinfo->available = true;
      tinfo->oldContext.setupBackup(&stackStart[perStackSize * 2 *i]);
      tinfo->newContext.setupBackup(&stackStart[perStackSize * 2 *i + 1]);
      WRAP(pthread_mutex_init)(&tinfo->mutex, NULL);
      WRAP(pthread_cond_init)(&tinfo->cond, NULL);
    }

    // Initialize the total event list.
    listInit(&_deferSyncs);
  }

  void finalize(void) {
  }

  /// @ internal function: allocation a thread index when spawning.
  /// Since we guarantee that only one thread can be in spawning phase,
  /// there is no need to acqurie the lock here.
  int allocThreadIndex() {
    int index = -1;

    if(_aliveThreads >= _totalThreads) {
      return index;
    } 

    int origindex = _threadIndex;
    thread_t * thread;
    while(true) {  
      thread = getThreadInfo(_threadIndex);
      if(thread->available) {
        thread->available = false;
        index = _threadIndex;
        
        // A thread is counted as alive when its structure is allocated.
        _aliveThreads++;

        _threadIndex = (_threadIndex+1)%_totalThreads;
        WRAP(pthread_mutex_init)(&thread->mutex, NULL);
        WRAP(pthread_cond_init)(&thread->cond, NULL);
    
//        fprintf(stderr, "origindex %d _threadindex %d (_threadIndex+1) %d - last %d\n", origindex, _threadIndex, (_threadIndex+1), (_threadIndex+1)%_totalThreads);
        break;
      }
      else {
        _threadIndex = (_threadIndex+1)%_totalThreads;
      }
    
      // It is impossible that we search the whole array and we can't find
      // an available slot. 
      assert(_threadIndex != origindex); 
    }
    return index;
  }
  
  inline thread_t * getThreadInfo(int index) {
    return &_threads[index];
  }

  inline thread_t * getThread(pthread_t thread) {
    return threadmap::getInstance().getThreadInfo(thread);
  }

  inline char * getThreadBuffer(int index) {
    thread_t * thread = getThreadInfo(index);

    return thread->outputBuf;
  }

  inline int incrementReapableThreads(void) {
    _reapableThreads++;
  }
  
  inline int hasReapableThreads(void) {
    return _reapableThreads != 0;
  } 

  inline void insertDeadThread(thread_t *thread) {
    deferSync((void *)thread, E_SYNCVAR_THREAD);
  }
 
  // Insert a synchronization variable into the global list, which 
  // are reaped later at commit points.
  inline void deferSync(void * ptr, syncVariableType type) {
    struct deferSyncVariable * syncVar = NULL;
    
    syncVar = (struct deferSyncVariable *)InternalHeap::getInstance().malloc(sizeof(struct deferSyncVariable));

    if(syncVar == NULL) {
      fprintf(stderr, "No enough private memory, syncVar %p\n", syncVar);
      return;
    }

    listInit(&syncVar->list);
    syncVar->syncVarType = type;
    syncVar->variable = ptr;
    
    global_lock();

    listInsertTail(&syncVar->list, &_deferSyncs);
    if(type == E_SYNCVAR_THREAD) {
      incrementReapableThreads();
    }   
 
    global_unlock();
  }

  void cancelAliveThread(pthread_t  thread) {
    thread_t * deadThread = getThread(thread);

    global_lock();
    
    threadmap::getInstance().removeAliveThread(deadThread);
    _aliveThreads--;
    _reapableThreads--;
    global_unlock();
  }

    // We actually get those parameter about new created thread
/* 
    E_SYNCVAR_THREAD = 0,
    E_SYNCVAR_COND,
    E_SYNCVAR_MUTEX,
    E_SYNCVAR_BARRIER
*/
  void runDeferredSyncs(void) {
    list_t * entry;

    // Get all entries from _deferSyncs.
    while((entry = listRetrieveItem(&_deferSyncs)) != NULL) {
      struct deferSyncVariable * syncvar = (struct deferSyncVariable *)entry;

      switch(syncvar->syncVarType) {
        case E_SYNCVAR_THREAD:
        {
          threadmap::getInstance().removeAliveThread((thread_t *)syncvar->variable);
          _aliveThreads--;
          _reapableThreads--;
          break;
        }

        case E_SYNCVAR_COND:
        {
          WRAP(pthread_cond_destroy)((pthread_cond_t *)syncvar->variable);
          break;
        }

        case E_SYNCVAR_MUTEX:
        {
          WRAP(pthread_mutex_destroy)((pthread_mutex_t *)syncvar->variable);
          break;
        }

        case E_SYNCVAR_BARRIER:
        {
          //int * test = (int *)syncvar->variable;
          //fprintf(stderr, "calling destory on %p\n", syncvar->variable);
          //fprintf(stderr, "Destroy barrier %p First Word %lx, second word %lx\n", syncvar->variable, test[0], test[1]);
          WRAP(pthread_barrier_destroy)((pthread_barrier_t *)syncvar->variable);
          break;
        }

        default:
          assert(0);
          break;
      }
      

      // Free this entry.
      InternalHeap::getInstance().free((void *)entry);
    }
   
    listInit(&_deferSyncs); 
  }
private:
  int     _aliveThreads;      // a. How many alive threads totally.
  int     _reapableThreads;      // a. How many alive threads totally.
  int     _totalThreads;      // b. How many alive threads we can hold 
  int     _threadIndex;        // b. What is the current thread index.
 // list_t  _aliveList;    // List of alive threads.
 // list_t  _deadList;     // List of dead threads.
  list_t  _deferSyncs; // deferred synchronizations. 
 // pthread_mutex_t _mutex; // Mutex to protect these list.
  thread_t    _threads[xdefines::MAX_ALIVE_THREADS];
/*
  char * position;     // c. What is the global heap metadata.
  size_t remainingsize; //
  h. User thread mapping address.
  i. Maybe the application text start and text end.
  j. Those mapping address
*/
};

#endif
