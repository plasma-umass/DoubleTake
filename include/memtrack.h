// -*- C++ -*-

/*
  Copyright (c) 2012 , University of Massachusetts Amherst.

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
#ifndef _memtrack_H_
#define _memtrack_H_
#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#include "internalheap.h"
#include "real.h"
#include "hashfuncs.h"
#include "hashmap.h"
#include "callsite.h"

typedef enum e_memtrackType{ 
  MEM_TRACK_MALLOC = 1,
  MEM_TRACK_FREE = 2
} memTrackType;

  
class trackObject {
  public:
    void * start;
    size_t objectSize; // Object size, mostly using for memory leak only.
		size_t currentObjectSize;  
    int    objecttype;
    int    tracktype; 
    // It is important to have this for use-after-free operation.
    // For example, an object can be re-used. It is possible that 
    // a object are going to use before this, then we do not report any
    // message on this.
    int tracedOps;
    CallSite allocSite;
    CallSite freeSite;
 
    trackObject() {}
 
    void setup(void * addr, size_t sz, faultyObjectType type, bool objectExist) {
      assert(addr != NULL);

      start = addr;
      objectSize = sz;

      if(objectExist) {
        objecttype |= type;
      }
      else {
        objecttype = type;
        tracktype = MEM_TRACK_MALLOC;
      }
        
      if(type == OBJECT_TYPE_USEAFTERFREE) {
        tracktype |= MEM_TRACK_FREE;
      }

      tracedOps = 0;
    }
  
    bool hasUseafterfree() {
      return (objecttype & OBJECT_TYPE_USEAFTERFREE) != 0;
    }

    bool hasLeak() {
      //PRINT("check has leak: tracktype %d(%b)\n", tracktype, tracktype);
      return (objecttype & OBJECT_TYPE_LEAK) != 0;
    }

    bool hasOverflow() {
      return (objecttype & OBJECT_TYPE_OVERFLOW) != 0;
    }

    void saveCallsite(size_t size, memTrackType type, int depth, void ** callsites) {
      if(type == MEM_TRACK_MALLOC) {
        allocSite.save(depth, callsites);
				currentObjectSize = size;
      }
      else {
        freeSite.save(depth, callsites);
      }

			tracedOps = type;
    }

		bool isInsideObject(void * addr) {
			return((((char *)addr) >= ((char *)start)) && (((char *)addr) <= ((char *)start) + currentObjectSize));
		}

    bool isMalloced() {
      return (tracedOps == MEM_TRACK_MALLOC);
    }

    bool isFreed() {
      //fprintf(stderr, "tracedOps %lx to isFreeTraced\n", tracedOps);
      return (tracedOps == MEM_TRACK_FREE);
    }
};

class memtrack {

public:
  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here, for use by the
  // signal handler.
  static memtrack& getInstance() {
    static char buf[sizeof(memtrack)];
    static memtrack * theOneTrueObject = new (buf) memtrack();
    return *theOneTrueObject;
  }

  memtrack()
  : _initialized(false) 
  {
  }

  void initialize(void) {
    _initialized = true;
    // Initialize a global track map.
    _trackMap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::MEMTRACK_MAP_SIZE); 
//    fprintf(stderr, "initializee!!!!!!!!!!!!!!\n");
  }

  void insert(void * start, size_t size, faultyObjectType type) {
    if(!_initialized) {
      initialize();
    }

    // When an overflow flushs several object, then there is no need to track all 
    // overflows.
    if((start == NULL) && type == OBJECT_TYPE_OVERFLOW) {
      PRINT("An overflow with start address NULL, not need to track.\n");
      return;
    }
 
    // Check whether tracktype is valid. 
    trackObject * object = NULL;
    bool objectExist = false;
     
    //PRINF("TRACKING object NNNNNNNOw: start %p size %d\n", start, size);
    // Check whether an object is existing or not. 
    // A object can has mulitple errors. 
    if(_trackMap.find(start, sizeof(start), &object)) {
      objectExist = true;
    }
    else {
      object = (trackObject *)InternalHeap::getInstance().malloc(sizeof(trackObject));
      _trackMap.insert(start, sizeof(start), object);
    }

   	//PRINF("Insert object at start %p\n", start);
    object->setup(start, size, type, objectExist);
  }

  // Check whether an object should be reported or not. Type is to identify whether it is 
  // a malloc or free operation. 
  // Then we can match to find whether it is good to tell
  void check(void * start, size_t size, memTrackType type);
  void print(void * start, faultyObjectType type);
	faultyObjectType getFaultType(void * start, void * faultyaddr);

private:

  bool _initialized; 
  HashMap<void *, trackObject *, spinlock, InternalHeapAllocator> _trackMap;
};


#endif /* _memtrack_H_ */
