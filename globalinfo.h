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
#include "threadstruct.h"
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
    _numOfEnds = 0;

    WRAP(pthread_mutex_init)(&_mutex, NULL);
    WRAP(pthread_cond_init)(&_condCommitter, NULL);
    WRAP(pthread_cond_init)(&_condWaiters, NULL);
  }
 
  void setEpochEnd(void) {
    _numOfEnds++;
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
    setRollback();

    // Wakeup all other threads.
    WRAP(pthread_cond_broadcast)(&_condWaiters);
  }

  void epochBegin(void) {
    lock();
    _phase = E_SYS_EPOCH_BEGIN;

    PRDBG("waken up all waiters\n");
    // Wakeup all other threads.
    WRAP(pthread_cond_broadcast)(&_condWaiters);

    if(_waiters != 0) {
      WRAP(pthread_cond_wait)(&_condCommitter, &_mutex);
    }
    unlock();
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
  }

  // Waiting for the stops of threads.
  void waitThreadsStops(void) {
    lock();
    while(_waiters != _waitersTotal) {
      WRAP(pthread_cond_wait)(&_condCommitter, &_mutex);
    }
    unlock();
  }

  void checkWaiters(void) {
    assert(_waiters == 0);
  }

  void incrementWaiters(void) {
    lock();
    PRDBG("waitForNotification _waiters %d totalWaiters %d\n", _waiters, _waitersTotal);

    _waiters++;
    if(_waiters == _waitersTotal) {
      WRAP(pthread_cond_signal)(&_condCommitter); 
    }

    unlock();
  }

  void decrementWaiters(void) {
    lock();
    _waiters--;
    unlock();
  }
 
  // Notify the commiter and wait on the global conditional variable 
  void waitForNotification(void) {
    assert(isEpochEnd() == true);

    lock();
    PRDBG("waitForNotification _waiters %d totalWaiters %d\n", _waiters, _waitersTotal);

    _waiters++;

    if(_waiters == _waitersTotal) {
      WRAP(pthread_cond_signal)(&_condCommitter); 
      PRDBG("waitForNotification after calling cond_broadcast\n");
    }

    // Only waken up when it is not the end of epoch anymore.
    while(isEpochEnd()) {
      PRDBG("waitForNotification before waiting again\n");
      WRAP(pthread_cond_wait)(&_condWaiters, &_mutex);
      PRDBG("waitForNotification after waken up. isEpochEnd() %d \n", isEpochEnd());
    }

    _waiters--;

    if(_waiters == 0) {
      WRAP(pthread_cond_signal)(&_condCommitter); 
    }
    PRDBG("waitForNotification decrement waiters. status %d\n", _phase);
      
    unlock();
  }

private:
  bool _isRollback;
  bool _hasRollbacked;
  int  _numOfEnds;
  enum SystemPhase _phase; 
  
  pthread_cond_t _condCommitter;
  pthread_cond_t _condWaiters;
  pthread_mutex_t _mutex;
  int _waiters;  
  int _waitersTotal;  
};

#endif
