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
#include "internalheap.h"
#include "recordentries.h"
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
    _objects.initialize();
  }

  void cacheFreeObject(void * ptr, int tindex) {
    struct freeObject * obj;
    lock();
    obj = _objects.alloc();
    unlock();

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
    return _objects.getIterEntry();
  }

private:
  void lock(void) {
    _lck.lock();
  }

  void unlock(void) {
    _lck.unlock();
  }

  spinlock _lck;
  RecordEntries<struct freeObject, xdefines::MAX_FREE_OBJECTS>  _objects;
};

#endif
