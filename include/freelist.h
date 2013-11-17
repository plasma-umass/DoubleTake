// -*- C++ -*-
/*
Copyright (c) 2012, University of Massachusetts Amherst.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

/*
* @file freelist.h
* @brief Managing heap owner information.
* @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
*/
#ifndef _FREE_LIST_H_
#define _FREE_LIST_H_

#include "xdefines.h"
#include "list.h"
//#include "internalheap.h"
#include "recordentries.h"
#include "quarantine.h"
#include "spinlock.h"

class freelist {

  freelist() {
  }

public:

  static freelist& getInstance (void) {
    static char buf[sizeof(freelist)];
    static freelist * theOneTrueObject = new (buf) freelist();
    return *theOneTrueObject;
  }
 
  void initialize(void) {
    _lck.init();
    _objects.initialize(xdefines::MAX_FREE_OBJECTS);
    //fprintf(stderr, "FREELIST _objects at %p******************************\n", &_objects);
  }

  objectHeader * getObject (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

  void cacheFreeObject(void * ptr, int tindex) {
    struct freeObject * obj;
  
    fprintf(stderr, "cacheFreeObject %p with tindex %d\n", ptr, tindex);
    lock();
    obj = _objects.alloc();
    unlock();
 
#ifdef DETECT_USAGE_AFTER_FREE 
    objectHeader * o = getObject(ptr);
    size_t size = o->getSize();
    markFreeObject(ptr, size);
#endif

    obj->ptr = ptr;
    obj->owner = tindex;
  }

  void preFreeAllObjects(void) {
    _objects.prepareIteration();
  }

  void postFreeAllObjects(void) {
    _objects.cleanup();
  }

  struct freeObject * retrieveFreeObject(void) {
    return _objects.retrieveIterEntry();
  }

#ifdef DETECT_USAGE_AFTER_FREE
  bool checkUAF(void) {
    bool hasUAF = false;
    int UAFErrors = 0;

    preFreeAllObjects();

    struct freeObject * object;

    while(object = retrieveFreeObject()) {
      fprintf(stderr, "Object is %p ptr %p\n", object, object->ptr);
      while(1);
      objectHeader * o = getObject(object->ptr);
      size_t size = o->getSize();
      if(hasUsageAfterFree(object, object->size)) {
        UAFErrors++;
        hasUAF = true;

        if(UAFErrors == 4) {
          break;
        }
      }
    }  
    return hasUAF;  
  }
#endif

private:
  void lock(void) {
    _lck.lock();
  }

  void unlock(void) {
    _lck.unlock();
  }

  spinlock _lck;
  RecordEntries<struct freeObject> _objects;
};

#endif
