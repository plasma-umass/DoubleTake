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
 * @file   objectmap.h
 * @brief  Used for leakage detection.  
           We has to design the storage of alive objects carefully. 
           For every possible value, we has to check alive objects.
           We are maintaining a slot based maps to hold all aliveobjects. 
           For example, we are diving the whole heap objects based on 1M. 
           For all objects inside each 1M, we maintains a map on those alive objects. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _ALIVE_OBJECTS_H_
#define _ALIVE_OBJECTS_H_

#include <set>

#include "xdefines.h"
#include "bitmap.h"
#include "mm.h"

class objectmap {
 
  objectmap(void) { }
 
public:
  static objectmap& getInstance (void) {
    static char buf[sizeof(objectmap)];
    static objectmap * theOneTrueObject = new (buf) objectmap();
    return *theOneTrueObject;
  }

  void initialize(void * start, void * end, size_t sz) {
    _heapBegin = (intptr_t)start;
    _heapEnd = (intptr_t)start;

    int mapsize = sz/xdefines::OBJECT_SIZE_BASE
    // Initialize the global bitmap.
    
  }

  inline int getBucket(unsigned long start) {
    return (start- _heapBegin)/_unitSize;
  }

  // We have to re-initialize for every time since start and end are different. 
  void initChunk(void * start, void * end, size_t sz) {
    int size;
    int bucket = getBucket((intptr_t)start); 
    
    _heapEnd = (intptr_t)end;
    int units = sz/_unitSize;
    int number = xdefines::MAX_OBJECTS_USER_HEAP_CHUNK;

    // Initialize the entries to record objects in this heap chunk
    if(units > 1) {
      number = 1;
    }

    // We actually don't need to record multiple entries in this case.
    for(int i = 0; i < units; i++) {
      _entries[bucket+i].initialize(number);  
    }
  }

  void insertObject(void * begin, void * end) {
    int bucket = getBucket((intptr_t)begin);
    struct object * obj;

    obj = _objects[bucket].alloc();

    assert(obj != NULL);
    obj->begin = (intptr_t)begin;
    obj->end = (intptr_t)end;
    obj->checked = false;
  }


  unsigned long getBucketStart(int bucket) {
    return(_heapBegin + bucket * _unitSize);
  }

  inline int getMin(int a, int b) {
    return (a >= b ? b : a);
  }

private:

  // Used to identify where is starting address of an allocation
  bitmap _allocMap;

  // Checkmap only used in the end of an epoch,
  // It is copied from allocation map
  bitmap _checkMap;
  unsigned long _heapBegin;
  unsigned long _heapEnd;
  unsigned long _unitSize; 
  unsigned long _buckets; 
};
#endif
