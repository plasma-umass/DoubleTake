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
 * @file   globalinfo.h
 * @brief  some global information about this system, by doing this, we can avoid multiple copies.
 *         Also, it is very important to utilize this to cooperate multiple threads since pthread_kill
 *         actually can not convey additional signal value information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include <errno.h>

#include <stdlib.h>
#include "threadmap.h"
#include "xdefines.h"

class globalinfo {

  enum SystemPhase {
    E_SYS_INIT, // Initialization phase
    E_SYS_NORMAL_EXECUTION, 
    E_SYS_EPOCH_END, // We are just before commit.  
    E_SYS_ROLLBACK, // We are trying to rollback the whole system.
    E_SYS_EPOCH_BEGIN, // We have to start a new epoch when no overflow. 
  };

public:
  globalinfo()
  {
  }

  static globalinfo& getInstance (void) {
    static char buf[sizeof(globalinfo)];
    static globalinfo * theOneTrueObject = new (buf) globalinfo();
    return *theOneTrueObject;
  }

  void initialize(void) {
    _isRollback = false;
    _hasRollbacked = false;
    _phase = E_SYS_INIT;

    WRAP(pthread_mutex_init)(&_mutex, NULL);
    WRAP(pthread_cond_init)(&_cond, NULL);

    if(xdefines::MAX_ALIVE_THREADS > 1) {
      _isMultithreading = true;
    }
    else {
      _isMultithreading = false;
    }
  }
 
  bool isMultithreading(void) {
    return _isMultithreading;
  }
   
  void setEpochEnd(void) {
    _phase = E_SYS_EPOCH_END;
  }

  bool isInitPhase(void) {
    return _phase == E_SYS_INIT;
  }

  bool isEpochEnd(void) {
    return _phase == E_SYS_EPOCH_END;
  }

  bool isRollback(void) {
    return _phase == E_SYS_ROLLBACK;
  }

  bool isEpochBegin(void) {
    return _phase == E_SYS_EPOCH_BEGIN;
  }

  void setRollback(void) {
    _phase = E_SYS_ROLLBACK;
    _hasRollbacked = true;
  }

  bool hasRollbacked(void) {
    return _hasRollbacked;
  }

  void rollback(void) {
    _phase = E_SYS_ROLLBACK;

    // Wakeup all other threads.
    WRAP(pthread_cond_broadcast)(&_cond);
  }

  void epochBegin(void) {
    _phase = E_SYS_EPOCH_BEGIN;

    // Wakeup all other threads.
    WRAP(pthread_cond_broadcast)(&_cond);
  }

  thread_t * getCurrent(void) {
    return current; 
  }

  void lock(void) {
    WRAP(pthread_mutex_lock)(&_mutex);
  }

  void unlock(void) {
    WRAP(pthread_mutex_unlock)(&_mutex);
  } 

  void setWaiters(int totalwaiters) {
    _waitersTotal = totalwaiters;
    while(_waiters != 0) {
      WRAP(pthread_cond_wait)(&_cond, &_mutex);
    }
  }

  // Waiting for the stops of threads.
  void waitThreadsStops(void) {
    lock();
    while(_waiters != _waitersTotal) {
      WRAP(pthread_cond_wait)(&_cond, &_mutex);
    }
    unlock();
  }

  
  void waitForNotification(void) {
    assert(isEpochEnd() == true);

    lock();
    PRDBG("waitForNotification _waiters %d totalWaiters %d\n", _waiters, _waitersTotal);

    _waiters++;

    if(_waiters == _waitersTotal) {
      WRAP(pthread_cond_broadcast)(&_cond); 
    }

    // Only waken up when it is not the end of epoch anymore.
    while(isEpochEnd()) {
      WRAP(pthread_cond_wait)(&_cond, &_mutex);
    }

    _waiters--;
      
    WRAP(pthread_cond_broadcast)(&_cond); 
    unlock();
  }

private:
  bool _isRollback;
  bool _hasRollbacked;
  bool _isMultithreading;
  enum SystemPhase _phase; 
  pthread_cond_t _cond;
  pthread_mutex_t _mutex;
  int _waiters;  
  int _waitersTotal;  
};

#endif
