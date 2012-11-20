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
 * @file   sanitycheck.h
 * @brief  Check the sanity of heap. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 *         Adopted from Diehard project.
 */
#ifndef _SANITYCHECK_H_
#define _SANITYCHECK_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"
#include "objectheader.h"
#include "bitmap.h"

class sanitycheck {
  enum { OBJECT_HEADER_SIZE = sizeof(objectHeader) }; 

public:

  sanitycheck() {

  }

  ~sanitycheck() {

  }

  // The single instance of sanitycheck. We only need this for 
  // heap. 
  static sanitycheck& getInstance (void) {
    static char buf[sizeof(sanitycheck)];
    static sanitycheck * theOneTrueObject = new (buf) sanitycheck();
    return *theOneTrueObject;
  }

  // Initialize corresponding bitmap
  void initialize(void * start, size_t size) {
    // Initialize the actual bitmap
    _bitmap.initialize(start, size);
    _checkStart = start;
    _checkEnd = (void *)((intptr_t)start + size); 
  }

  // Set up the sentinels for the specified object.
  void setupSentinels(void * ptr, size_t objectsize) {
    size_t * sentinelFirst, * sentinelLast;
    
    // Calculate the address of two sentinels
    // The organization should be:
    //      objectHeader + sentinelFirst + object (objectsize) + sentinelLast
    sentinelFirst = (size_t *)((intptr_t)ptr + OBJECT_HEADER_SIZE); 
    sentinelLast = (size_t *)((intptr_t)ptr + OBJECT_HEADER_SIZE + xdefines::SENTINEL_SIZE + objectsize);
    *sentinelFirst = xdefines::SENTINEL_WORD;
    *sentinelLast = xdefines::SENTINEL_WORD;
  
    fprintf(stderr, "SET sentinels: first %p last %p\n", sentinelFirst, sentinelLast); 
    // Now we have to set up corresponding bitmap so that we can check heap overflow sometime
    _bitmap.tryToSet((void *)sentinelFirst);  
    _bitmap.tryToSet((void *)sentinelLast);  
  }

  // Check the integrity of heap.
  bool checkHeapIntegrity(void * start, size_t size) {
    //fprintf(stderr, "checkHeapIntegrity: start %p size %x. _checkStart %p heapPos %p\n", start, size, _checkStart, _heapPos); 
    if(start >= _checkStart && start <= _heapPos) {
      return _bitmap.checkSentinelsIntegrity(start, size);
    }
    else {
      return false;
    }
  } 

  void setHeapPosition(void * addr) {
    _heapPos = addr;
  }

  void cleanup(void * start, size_t sz) {
    _bitmap.cleanup(start, sz);
  }

private:
  // start address of sanity check (start of actual heap)
  void * _checkStart;
  void * _checkEnd;

  // What is the current position of heap
  void * _heapPos;
 
  // Pointing to a shared bitmap
  bitmap _bitmap;
};

#endif
