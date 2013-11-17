
/*
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
 * @file   determ.h
 * @brief  Main file for determinism management.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Charlie Curtsinger <http://www.cs.umass.edu/~charlie>
 */
#ifndef __DETERM_H__
#define __DETERM_H__


#include <map>

#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include "xdefines.h"
#include "list.h"

// We are using a circular double linklist to manage those alive threads.
class determ {
public:
  determ()
  {  
  }

  static determ& getInstance (void) {
    static determ * determObject = NULL;
    if(!determObject) {
      void *buf = WRAP(mmap)(NULL, sizeof(determ), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
      determObject = new(buf) determ();
    }
    return * determObject;
  }

  void initialize(void) {
    // Set up with a shared attribute.
    WRAP(pthread_mutexattr_init)(&_mutexattr);
    pthread_mutexattr_setpshared(&_mutexattr, PTHREAD_PROCESS_SHARED);
    WRAP(pthread_condattr_init)(&_condattr);
    pthread_condattr_setpshared(&_condattr, PTHREAD_PROCESS_SHARED);

    // Initialize the mutex.
    WRAP(pthread_mutex_init)(&_mutex, &_mutexattr);
    WRAP(pthread_cond_init)(&_cond, &_condattr);

    _arrival_phase = true;
    _maxthreads = 0;
    _arrivals = 0;
  }

  // Increment the fence when all threads has been created by current thread.
  void startFence(int threads) {
    lock();
    _maxthreads += threads;
    unlock();
  }

  // Increase the fence, not need to hold lock!
  void incrFence(int num) {
    _maxthreads += num;
  }

  void decrFence(void) {
    _maxthreads--;
  }

  // Here, a fence for internal usage in order to support the repeatable replay.
  void waitFence( ) {
    int retvalue;

    if(_maxthreads == 1) {
      return;
    }

    // Everyone is in the exiting phase, it should not take too long
    while(_arrival_phase != true) {
      ; 
    }

    lock();

    // Now in an arrival phase, proceed with barrier synchronization
    _arrivals++;
   
    // Whenever all threads arrived in the barrier, wakeup everyone on the barrier. 
    if(_arrivals == _maxthreads) {
      // Now every threads is leaving this fence.
      _arrival_phase = false;

      // Wakeup everybody on the conditional variable.
      WRAP(pthread_cond_broadcast)(&_cond);
    } else {
      // Wait on fence if some is not at fence
      while(_arrival_phase == true) {
      //  entry->wait = 1;
        WRAP(pthread_cond_wait)(&_cond, &_mutex);
      }
    }
   
    // Mark one thread is leaving the barrier. 
    _arrivals--;
  
    // When all threads leave the barrier, entering into the new arrival phase.  
    if(_arrivals == 0) {
      _arrival_phase = true;
    }

    unlock();

    return;
  }
 
  inline void setSpawningPhase(void) {
    _spawning_phase = true;
  }
 
  // A child is waiting the parent to wake it up after spawning 
  // in order to guarantee the determinism.
  inline void waitParentNotify(void) {
    lock();

    // Parent will set spawning phase to false when it is to 
    // waken up all children.  
    while(_spawning_phase == true) { 
      WRAP(pthread_cond_wait)(&_cond_spawning, &_mutex);
    }

    unlock();
  }

  // The parent is waken up all waiting children.
  inline void notifyWaitingChildren(void) {
    lock();
    _spawning_phase = false;
    WRAP(pthread_cond_broadcast)(&_cond_spawning);
    unlock();
  }


private:

  inline void lock(void) {
    WRAP(pthread_mutex_lock)(&_mutex);
  }
  
  inline void unlock(void) {
    WRAP(pthread_mutex_unlock)(&_mutex);
  }
  
  pthread_mutex_t _mutex;
  pthread_cond_t _cond;
  pthread_condattr_t _condattr;
  pthread_mutexattr_t _mutexattr;
  
  // Some conditional variable used for thread creation and joining.
  pthread_cond_t _cond_spawning;
  pthread_cond_t _cond_join;

  // Variables related to  fence control
  volatile size_t _maxthreads;
  volatile int _arrivals;
  volatile bool _arrival_phase;
  volatile int  _assigned_thread_index;
};

#endif
