// -*- C++ -*-

/*
  Copyright (c) 2011, University of Massachusetts Amherst.

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
 * @file   xpheap.h
 * @brief  A heap optimized to reduce the likelihood of false sharing.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XPHEAP_H_
#define _XPHEAP_H_

#include "xdefines.h"
#include "sassert.h"
#include "ansiwrapper.h"
#include "kingsleyheap.h"
#include "adapt.h"
#include "sllist.h"
#include "dllist.h"
#include "zoneheap.h"
#include "objectheader.h"
#include "sanitycheckheap.h"
#include "spinlock.h"

// Implemented for stopgap to check buffer overflow
#include "sanitycheck.h"

template <class SourceHeap>
class NewSourceHeap : public SourceHeap {

public:
  void * malloc (size_t sz) {

    // We are adding one objectHeader and two "canary" words along the object
    // The layout will be:  objectHeader + "canary" + Object + "canary".
    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader) + 2 * xdefines::SENTINEL_SIZE);
    if (!ptr) {
      return NULL;
    }
  
  //  fprintf(stderr, "NewSourceHeap: sz is %d, actual size %d, sizeof size_t is %d\n", sz, sz+sizeof(objectHeader), sizeof(size_t)); 
    // Set the objectHeader
    objectHeader * o = new (ptr) objectHeader (sz);

    // Now we are adding two sentinels and mark them on the shared bitmap.
    sanitycheck::getInstance().setupSentinels(ptr, sz); 
   
    void * newptr = getRealPointer(o);

    assert (getSize(newptr) >= sz);
   // fprintf(stderr, "NEWSOURCEHEAP: sz is %x - %d, newptr %p\n", sz, sz, newptr); 
    return newptr;
  }

  void free (void * ptr) {
    SourceHeap::free ((void *) getObjectHeader(ptr));
  }

  size_t getSize (void * ptr) {
    objectHeader * o = getObjectHeader(ptr);
    size_t sz = o->getSize();
    if (sz == 0) {
      PRFATAL ("Object size error, can't be 0");
    }
    return sz;
  }

private:

  // Get the object header when object is free
  static objectHeader * getObjectHeader (void * ptr) {
    objectHeader * o = (objectHeader *)((intptr_t)ptr - sizeof(objectHeader) - xdefines::SENTINEL_SIZE);
    return o;
  }

  // It is called to get the start address of actual object 
  static void * getRealPointer (void * ptr) {
    return (void *) ((intptr_t)ptr + sizeof(objectHeader) + xdefines::SENTINEL_SIZE);
  }
};

template <class SourceHeap, int Chunky>
class KingsleyStyleHeap :
  public 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, NewSourceHeap<SourceHeap> >,
		    NewSourceHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
{
private:

  typedef 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, NewSourceHeap<SourceHeap> >,
		    NewSourceHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
  SuperHeap;

public:
  KingsleyStyleHeap (void) {
  }

  void * malloc (size_t sz) {
    void * ptr = SuperHeap::malloc (sz);
    return ptr;
  }

private:
  // We want that a single heap's metadata are on different page
  // to avoid conflicts on one page
  char buf[4096 - (sizeof(SuperHeap) % 4096)];
};

// Different processes will have a different heap.
//class PerProcessHeap : public TheHeapType {
template <int NumHeaps,
	  class TheHeapType>
class PerProcessHeap {
public:
  PerProcessHeap (void)
  {
#if 0
    fprintf(stderr, "sizeof TheHeapType is %lx\n", sizeof(TheHeapType));
    for(int i = 0; i < NumHeaps;i++)
    fprintf(stderr, "PERPROCESSHEAP: %d _heap  is at %p. _heap itself %p\n",i, &_heap[i], &_heap);
#endif 
  }

  void * malloc (int ind, size_t sz)
  {
    // Try to get memory from the local heap first.
    void * ptr = _heap[ind].malloc (sz);
    return ptr;
  }
 
  // Here, we will give one block of memory back to the originated process related heap. 
  void free (int ind, void * ptr)
  { 
    //int ind = getHeapId(ptr);
    if(ind >= NumHeaps) {
      PRERR("wrong free status\n");
    }
    _heap[ind].free (ptr);
    //fprintf(stderr, "now first word is %lx\n", *((unsigned long*)ptr));
  }

  // For getSize, it doesn't matter which heap is used 
  // since they are the same
  size_t getSize (void * ptr) {
    return _heap[0].getSize (ptr);
  }

private:
  TheHeapType _heap[NumHeaps];
};

// Protect heap 
template <class SourceHeap>
class xpheap : public SourceHeap 
{
  typedef PerProcessHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap, xdefines::PHEAP_CHUNK> >
  SuperHeap;

public: 
  xpheap() {
  }

  void initialize(void) {
    // Initialize the SourceHeap before malloc from there.
    SourceHeap::initialize(NULL, xdefines::PROTECTEDHEAP_SIZE);

    int  metasize = sizeof(SuperHeap);

    char * base;

    base = (char *)SourceHeap::malloc(metasize);
    if(base == NULL) {
      PRFATAL("Failed to allocate memory for heap metadata.");
    }
    _heap = new (base) SuperHeap;
    
    // Get the heap start and heap end;
    _heapStart = SourceHeap::getHeapStart();
    _heapEnd = SourceHeap::getHeapEnd();

    _sanitycheckStart = (void *) ((intptr_t)(_heapStart) + metasize);
    _sanitycheckSize = xdefines::PROTECTEDHEAP_SIZE -  metasize;
    //fprintf(stderr, "XPHEAP: size %d base %p at %p\n", metasize, _heap, &_heap);
  }


  // Initialization for sanity check.
  void sanitycheckInitialize(void) {
    sanitycheck::getInstance().initialize(_sanitycheckStart, _sanitycheckSize);
  }

#if 1
  // Performing the actual sanity check.
  bool sanitycheckPerform(void) {
    void * heapEnd =(void *)SourceHeap::getHeapPosition();
    sanitycheck::getInstance().setHeapPosition(heapEnd);
    return SourceHeap::sanitycheckPerform();
  }
#endif

  void * malloc(int heapid, size_t size) {
    _heap->malloc(heapid, size);
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

  // Sanity check related information
  void * _sanitycheckStart;
  size_t _sanitycheckSize;
};

#endif // _XPHEAP_H_

