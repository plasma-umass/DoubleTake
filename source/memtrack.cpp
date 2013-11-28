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
#include "memtrack.h"

// Check whether an object should be reported or not. Type is to identify whether it is 
// a malloc or free operation. 
// Then we can match to find whether it is good to tell
void memtrack::check(void * start, size_t size, memTrackType type) {
  bool toGetCallsite = false;
  bool toPrintCallsite = false;
  // For a lot of case, maybe we only update corresponding data structure here. 
  trackObject *object;

  if(!_initialized) {
    return;
  }
 
  if(_trackMap.find(start, sizeof(start), &object)) {
    // Now we should verify the size information for buffer overflow and memory leakage.
    // For use-after-free errors, since it is impossible to know actual object size information,
    // then we do not verify its size information.
    if((object->objecttype == OBJECT_TYPE_USEAFTERFREE) | (object->objectSize == size)) {
      // Now we check the type of this object.
      switch(object->objecttype) {
        case OBJECT_TYPE_OVERFLOW:
          {
            if(type == MEM_TRACK_MALLOC) {
              // Record the callsite information here.
              toGetCallsite = true;
            }
          }
          break;

        case OBJECT_TYPE_USEAFTERFREE:
          {
             toGetCallsite = true;
          }
          break;
      
        case OBJECT_TYPE_LEAK:
          {
            if(type == MEM_TRACK_MALLOC) {
              toGetCallsite = true;
              toPrintCallsite = true;
            }
          }
          break;
        
        default:
          assert(0);
      }

      if(toGetCallsite) {
        void * callsites[xdefines::CALLSITE_MAXIMUM_LENGTH]; 
        int depth = selfmap::getCallStack((void **)&callsites);

        // Insert or print.
        if(toPrintCallsite) {
          assert(object->objecttype == OBJECT_TYPE_LEAK);
          PRINT("\nLeak object at address %p size %ld. Current call stack:\n", object->start, object->objectSize);
          selfmap::getInstance().printCallStack(depth, (void **)&callsites[0]); 
        } 
        else {
          // save this callsite.
          object->saveCallsite(type, depth, (void **)&callsites[0]);
        }
      }
    }
  }

}

void memtrack::print(void * start) {
  // For a lot of case, maybe we only update corresponding data structure here. 
  trackObject *object;

  assert(_initialized == true); 

  // Find corresponding object 
  if(_trackMap.find(start, sizeof(start), &object)) {
    // Now we should verify the size information for buffer overflow and memory leakage.
    // For use-after-free errors, since it is impossible to know actual object size information,
    // then we do not verify its size information.
    PRINT("Memory allocation call stack:\n");
    assert(object->isMallocTraced() == true);
     
    // Print its allocation stack.     
    selfmap::getInstance().printCallStack(object->allocSite.depth(), object->allocSite.getCallsite()); 
    
    if(object->objecttype == OBJECT_TYPE_USEAFTERFREE) {
      assert(object->isFreeTraced() == true);
      PRINT("Memory deallocation call stack:\n");
      selfmap::getInstance().printCallStack(object->freeSite.depth(), object->freeSite.getCallsite()); 
    }
  }
}
