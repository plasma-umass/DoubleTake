// -*- C++ -*-

/*
  Copyright (C) 2011 University of Massachusetts Amherst.

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

#ifndef _INTERNALHEAP_H_
#define _INTERNALHEAP_H_

#include "xdefines.h"
#include "xpheap.h"
#include "xoneheap.h"
#include "sourceinternalheap.h"
/**
 * @file InternalHeap.h
 * @brief A share heap for internal allocation needs.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *
 */
template <class SourceHeap>
class perheap : public SourceHeap 
{
  typedef PerThreadHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap, xdefines::INTERNAL_HEAP_CHUNK> >
  SuperHeap;

public: 
  perheap() {
  }

  void initialize(void) {

    int  metasize = alignup(sizeof(SuperHeap), xdefines::PageSize);

    // Initialize the SourceHeap before malloc from there.
    char * base = (char *) SourceHeap::initialize((void *)xdefines::INTERNAL_HEAP_BASE, xdefines::INTERNAL_HEAP_SIZE, metasize);
  
    if(base == NULL) {
      PRFATAL("Failed to allocate memory for heap metadata.");
    }
//    fprintf(stderr, "\n\nInternalHeap base %p metasize %lx\n\n", base, metasize);
    _heap = new (base) SuperHeap;
    
    // Get the heap start and heap end;
    _heapStart = SourceHeap::getHeapStart();
    _heapEnd = SourceHeap::getHeapEnd();
  }

  void * malloc(int heapid, size_t size) {
    return _heap->malloc(heapid, size);
  }

  void free(int heapid, void * ptr) {
    _heap->free(heapid, ptr);
  }

  size_t getSize (void * ptr) {
    return _heap->getSize (ptr);
  }

  bool inRange(void * addr) {
    return ((addr >= _heapStart) && (addr <= _heapEnd)) ? true : false;
  }
 
private:
  SuperHeap * _heap;
  void * _heapStart;
  void * _heapEnd; 
};


class InternalHeap { 

public:

  InternalHeap() {}

  void initialize()
  {
    _heap.initialize();
  }
 
  virtual ~InternalHeap (void) {}
  
  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here, for use by the
  // signal handler.
  static InternalHeap& getInstance (void) {
    static char buf[sizeof(InternalHeap)];
    static InternalHeap * theOneTrueObject = new (buf) InternalHeap();
    return *theOneTrueObject;
  }
  
  void * malloc (size_t sz) {
    void * ptr = NULL;
    ptr = _heap.malloc (getThreadIndex(), sz);
  
    if(!ptr) {
      PRERR("%d : SHAREHEAP is exhausted, exit now!!!", getpid());
      assert(ptr != NULL);
    }
  
    return ptr;
  }
  
  void free (void * ptr) {
    _heap.free (getThreadIndex(), ptr);
  }
  
private:
  perheap<xoneheap<SourceInternalHeap > >  _heap;  
};


#endif
