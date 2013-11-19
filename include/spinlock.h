// -*- C++ -*-

/*
  Copyright (c) 2012-2013, University of Massachusetts Amherst.

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
 * @file:   spinlock.h
 * @brief:  spinlock used internally.
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *  Note:   Some references: http://locklessinc.com/articles/locks/   
 */
#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "atomic.h"

class spinlock {
public:
  spinlock() {
    _lock = 0;
  }

  void init() {
    _lock = 0;
  }

  // Lock 
  void lock() {
#if 0
    int cnt = 0;
	 
    while (atomic_test_and_set(&lock)) {
 	    if (cnt < MAX_SPIN_COUNT) {
	      cnt++;
	    }
      cnt = 0;
    }
#endif
#if 0
#define SPIN_SLEEP_DURATION 5000001
#define MAX_SPIN_COUNT 10000

  int cnt = 0;
  struct timespec tm;

  while (atomic_test_and_set(&_lock)) {
    if (cnt < MAX_SPIN_COUNT) {
      cnt++;
    } else {
      tm.tv_sec = 0;
      tm.tv_nsec = SPIN_SLEEP_DURATION;
      syscall(__NR_nanosleep, &tm, NULL);
      cnt = 0;
    }
  }

#else
  while (atomic_test_and_set(&_lock)) {
    //while(_lock) {
    // There is no need to check the _lock since it could be inside the register
      /* Pause instruction to prevent excess processor bus usage */ 
    atomic_cpuRelax();
   // }
  }
#endif
  }
  
  void unlock() {
    _lock = 0;
    atomic_memoryBarrier();
  }

private:
  volatile unsigned long _lock;
};

#endif /* __SPINLOCK_H__ */

