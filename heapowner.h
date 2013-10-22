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
 * @file   heapowner.h
 * @brief  Managing heap owner information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _HEAP_OWNER_H_
#define _HEAP_OWNER_H_

#include "xdefines.h"
#include "list.h"
#include "internalheap.h"

class heapowner {

  heapowner() {
  }

  struct bigHeapInfo {
    list_t list;
    void * start;
    void * end;
    int    owner; // which thread is using this heap.
  };

public:

  static heapowner& getInstance (void) {
    static char buf[sizeof(heapowner)];
    static heapowner * theOneTrueObject = new (buf) heapowner();
    return *theOneTrueObject;
  }
 
  void initialize(void * start, size_t size, size_t unitsize) {
    assert(start != NULL);

    listInit(&_bigHeapOwners);

    // Calculate how many units we should allocate
    int units = size/unitsize;
   
    _heapStart = start;
    _normalOwners = (int *)InternalHeap::getInstance().malloc(units * sizeof(int));
    
    for(int i = 0; i < units; i++) {
      _normalOwners[i] = -1; 
    }

    _unitSize = unitsize;
    _bigHeapStart = NULL;
  } 

  inline size_t getIndex(void * start) {
    return ((intptr_t)start - (intptr_t)_heapStart)/_unitSize;
  }

  inline void registerNormalHeap(void * start, int threadindex) {
    assert(((intptr_t)start - (intptr_t)_heapStart)%_unitSize == 0);   
 
    size_t index = getIndex(start);
    _normalOwners[index] = threadindex; 
  }

  inline void registerBigHeap(void * start, void * end, int threadindex) {
    struct bigHeapInfo * entry = NULL;
    entry = (struct bigHeapInfo *)InternalHeap::getInstance().malloc(sizeof(*entry));

    entry->start = start;
    entry->end = end;
    entry->owner = threadindex;

    listInsertTail(&entry->list, &_bigHeapOwners); 
  }

  
  inline int getHeapOwner(void * start) {
    int owner = -1;
    if(start > _bigHeapStart) {
      // Now we have to traverse the whole list in order to get
      // index for a specific entry.
      struct bigHeapInfo * entry = NULL;
      
      while(true) {
        entry = (struct bigHeapInfo *)listRetrieveItem(&_bigHeapOwners);

        if(entry == NULL) {
          PRERR("We can find an entry for start %p\n", start);
          break;
        }
        else if(start >= entry->start && start < entry->end) {
          owner = entry->owner;
          break;
        }
      } 
    }
    else {
      size_t index = getIndex(start);
      owner = _normalOwners[index];
    }
    return owner;
  }


private:
  size_t _unitSize;
  void * _heapStart;
  void * _bigHeapStart;
  int * _normalOwners;
  list_t _bigHeapOwners;
};

#endif
