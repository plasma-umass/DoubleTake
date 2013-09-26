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
 * @file   synceventpool.h
 * @brief  Managing the pool of synchronization events. The basic of having pool is 
           to reduce unnecessary memory allocation and deallocation operations, similar 
           to slab manager of Linux system. However, it is different here.
           There is no memory deallocation for each pool. 
           In the same epoch, we keep allocating from 
           this pool and udpate correponding counter, updating to next one.
           When epoch ends, we reset the counter to 0 so that we can reuse all 
           
            
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _SYNCEVENTPOOL__H_
#define _SYNCEVENTPOOL__H_

#include "xdefines.h"
#include "mm.h"

extern "C" {
  // Record corresponding information for each event.  
  struct syncEvent {
    list_t     list;
    // Which thread is performing synchronization? 
    void    *  thread;
    void    *  eventlist;
    int        ret; // used for mutex_lock
  };

};

class SyncEventPool {
public:
  SyncEventPool() {
  }

  void initialize(void) {
		void * ptr;
    int i = 0;

    _size = alignup(xdefines::SYNCEVENT_POOL_ENTRIES * sizeof(struct syncEvent), xdefines::PageSize);
		// We don't need to allocate all pages, only the difference between newnum and oldnum.
		ptr = MM::mmapAllocatePrivate(_size);
		if(ptr == NULL)  {
			fprintf(stderr, "%d fail to allocate sync event pool entries : %s\n", getpid(), strerror(errno));
			::abort();
		}
		
		// start to initialize it.
    _start = (struct syncEvent *)ptr;
		_cur = 0;
		_total = xdefines::SYNCEVENT_POOL_ENTRIES;
    _iter = 0;
		return;
	}

  void finalize(void) {
    MM::mmapDeallocate(_start, _size);
  }

	struct syncEvent * alloc(void) {
		struct syncEvent * entry = NULL;
		if(_cur < _total) {
			entry = &_start[_cur];
			_cur++;
		}
 		else {
			// There is no enough entry now, re-allocate new entries now.
			fprintf(stderr, "Not enought synchronization event entry, now _cur %x, _total %x!!!\n", _cur, _total);
			::abort();
		}
		return entry;
  }

	void cleanup(void) {
    _iter = 0;
		_cur = 0;
  }

  void prepareRollback(void) {
    _iter = 0;
  }

  inline struct syncEvent * getEntry(size_t index) {
    return &_start[index];
  }

  struct syncEvent * nextIterEntry(void) {
    _iter++;
    if(_iter < _cur) {
      return getEntry(_iter);
    }
    else {
      return NULL;
    }
  }

  // Only called in the replay
  struct syncEvent * firstIterEntry(void) {
    return &_start[_iter];
  }

private:
  struct syncEvent * _start; 
  size_t _total; 
  size_t _cur; //
  size_t _size; 
  size_t _iter; 
};

#endif
