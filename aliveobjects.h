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
 * @file   aliveobjects.h
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
#include "mm.h"
#include "stlallocator.h"

extern "C" {
  struct aliveObject {
    void * begin;
    void * end;
    // If it is checked, then it is not a leak object.
    mutable bool   checked; 
  };
};

class aliveobjects {
  
public:
  static aliveobjects& getInstance (void) {
    static char buf[sizeof(aliveobjects)];
    static aliveobjects * theOneTrueObject = new (buf) aliveobjects();
    return *theOneTrueObject;
  }

  // We have to re-initialize for every time since start and end are different. 
  void initialize(void * start, void * end) {
    _heapBegin = (intptr_t)start;
    _heapEnd = (intptr_t)end;

    size_t size = _heapEnd - _heapBegin;
    // We assign buckets according to minimum block size used in xheap.h
    _unitSize = xdefines::USER_HEAP_CHUNK + xdefines::PageSize;
    _buckets = size/_unitSize;

    // Now calculate how much memory we need to hold all these possible heap objects.
    _objects = (objectsSetType*)InternalHeapAllocator::malloc(sizeof(objectsSetType) * _buckets);
  }

  void finalize(void) {
    // free memory used to hold those objects.
    InternalHeapAllocator::free(_objects); 
  }

  // Adding an alive object into the table.
  void insertObject(void * begin, void * end) {
    struct aliveObject object;
    object.begin = begin;
    object.end = end;

    int bucket = getBucket(begin);

    _objects[bucket].insert(object);
  }

  // This has to related with the ownership design thus
  // we won't have an object which crosses two buckets. 
  int getBucket(void * ptr) {
    return ((intptr_t)ptr - _heapBegin)/_unitSize;
  }


  // If this pointer can be found in alive objects, return 
  // the actua; object. Otherwise, return NULL.
  //bool checkObject(void * ptr, struct aliveObject * object) {
  const struct aliveObject * checkObject(void * ptr) {
    const struct aliveObject * object = NULL;
    // Get bucket slot number;
    int bucket = getBucket(ptr);
    struct aliveObject thisObject;

    thisObject.begin = ptr;
    thisObject.end = ptr;

    objectsSetType::iterator i;
    i = _objects[bucket].find(thisObject);
    if(i != _objects[bucket].end()) {
      i->checked = true;
      object = &(*i);
      //object = (const struct aliveObject *)&(*i);
    } 
    return object;
  }

private:

  struct object_compare {
    /* strict-weak ordering has to meet the following three conditions.
      (x < x) == false
      (x < y) == !(y < x)
      ((x < y) && (y < z)) == (x < z)

      Example, there are two objects in the set:
      1-4 6-9

      If I input 5-5, then it is not existing.
      If input 6-6, then it returns second object.
      If inpput 3-3, then it returns first object.
    */
    bool operator() (const struct aliveObject & lhs, const aliveObject & rhs) const{
      if(lhs.begin < rhs.begin && lhs.end < rhs.end) {
        return true;
      }
      else {
        return false;
      } 
    }
  };

  typedef std::set<struct aliveObject, object_compare, HL::STLAllocator<struct aliveObject, InternalHeapAllocator> > objectsSetType;
  objectsSetType * _objects;


  unsigned long _heapBegin;
  unsigned long _heapEnd;
  unsigned long _unitSize; 
  unsigned long _buckets; 
};
#endif
