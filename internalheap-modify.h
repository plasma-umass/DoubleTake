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
/**
 * @file InternalHeap.h
 * @brief A share heap for internal allocation needs.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *
 */
class SourceInternalHeap {
public:
  SourceInternalHeap (void)
  {}

  void initialize(size_t startsize) 
  {
    fprintf(stderr, "Size of SourceInternalHeap is %x\n", sizeof(SourceInternalHeap)); 
	  // Allocate a share page to hold all heap metadata.
    char * base = (char *)MM::mmapAllocatePrivate(startsize);
    if(base == MAP_FAILED) {
      PRERR("Couldn't do mmap %s : sz %lx\n", strerror(errno), startsize);
      abort();
    }

	  // Put all "heap metadata" in this page.
    WRAP(pthread_mutex_init)(&_mutex, NULL);

    _position   = (char *)base;
    _remaining  = startsize;

    PRWRN("internalheap initialization: position %p (at %p) remaining %p\n", _position, &_position, _remaining);
  }

  // We need to page-aligned size, we don't want that
  // two different threads are using the same page here.
  inline void * malloc (size_t sz) {
    // Roud up the size to page aligned.
	  sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1) / xdefines::PageSize);
    
    PRWRN("internalheap malloc sz %d position %p (at %p) remaining %p\n", sz, _position, &_position, _remaining);
	  lock();

    if (_remaining < sz) { 
      PRFATAL("Fatal error: remaining %lx sz %lx\n", _remaining, sz);
    }

    void * p = _position;

    // Increment the bump pointer and drop the amount of memory.
    _remaining -= sz;
    _position += sz;

	  unlock();
    PRWRN("internalheap ptr %p position %p (at %p) remaining %p\n", p, _position, &_position, _remaining);
    
    return p;
  }

  // These should never be used.
  inline void free (void * ptr) { ; }
  inline size_t getSize (void * ptr) { return 0; } 
private:
  void lock() {
    WRAP(pthread_mutex_lock)(&_mutex);
  }

  void unlock() {
    WRAP(pthread_mutex_unlock)(&_mutex);
  }

  /// Pointer to the current bump pointer.
  char *  _position;

  /// Pointer to the amount of memory remaining.
  size_t  _remaining;

  // This lock guards allocation requests from different threads.
  pthread_mutex_t  _mutex;
};

template <class SourceHeap>
class AdaptInternalHeap : public SourceHeap {
public:
  void * malloc (size_t sz) {
    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader));
    if (!ptr) {
      return NULL;
    }
  
    // Set the objectHeader. 
    objectHeader * o = new (ptr) objectHeader (sz);
    void * newptr = getPointer(o);

    fprintf(stderr, "****AdaptInternalHeap newptr %p********\n", newptr);
    assert (getSize(newptr) == sz);
    return newptr;
  }

  void free (void * ptr) {
    SourceHeap::free ((void *) getObject(ptr));
  }

  size_t getSize (void * ptr) {
    objectHeader * o = getObject(ptr);
    size_t sz = o->getSize();
    if (sz == 0) {
      PRFATAL ("Object size error, can't be 0");
    }
    return sz;
  }

private:
  static objectHeader * getObject (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

  static void * getPointer (objectHeader * o) {
    return (void *) (o + 1);
  }
};

template <class SourceHeap, int Chunky>
class KingsleyStyleHeap2 :
  public 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, AdaptInternalHeap<SourceHeap> >,
		    AdaptInternalHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
{
private:

  typedef 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, AdaptInternalHeap<SourceHeap> >,
		    AdaptInternalHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
  SuperHeap;

public:
  KingsleyStyleHeap2 (void) {
    //fprintf(stderr, "KingsleyStyleHeap2 SuperHeap size %p\n", sizeof(SuperHeap));
  }

private:
  // We want that a single heap's metadata are on different page
  // to avoid conflicts on one page
  //char buf[4096 - (sizeof(SuperHeap) % 4096)];
};

extern "C" {
  int getHeapIndex();
};

class InternalHeap : SourceInternalHeap 
{
  typedef PerThreadHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap2<SourceInternalHeap, xdefines::PHEAP_CHUNK> >
  SuperHeap;

public: 
  InternalHeap() {
    //fprintf(stderr, "PerThreadHeap superheap size %p\n", sizeof(KingsleyStyleHeap2<SourceInternalHeap, xdefines::PHEAP_CHUNK>));
  }

  static InternalHeap& getInstance (void) {
    static void * buf[sizeof(InternalHeap)];
    static InternalHeap * theOneTrueObject = new (buf) InternalHeap();
    return *theOneTrueObject;
  }

  void initialize(void) {
    // Initialize the SourceInternalHeap before malloc from there.
    SourceInternalHeap::initialize(xdefines::INTERNAL_HEAP_SIZE);
    int  metasize = sizeof(SuperHeap);

    char * base;

    base = (char *)SourceInternalHeap::malloc(metasize);
    if(base == NULL) {
      PRFATAL("Failed to allocate memory for internal heap metadata.");
    }
    _heap = new (base) SuperHeap;
    printf("xpheap calling sourceHeap::malloc size %d _heap %p\n", metasize, _heap);
  }

  static void * allocate(size_t size) {
    return malloc(size);
  }

  static void deallocate(void * ptr) {
    free(ptr);
  }

  static void * malloc(size_t size) {
    PRWRN("getHeap %p\n", getHeap());
    PRWRN("getHeapIndex %d\n", getHeapIndex());
    // Acquire the heap index
    void * ptr =  getHeap()->malloc(getHeapIndex(), size);
    PRWRN("getHeapIndex %d ptr %p\n", getHeapIndex(), ptr);
    return ptr;
    //return getHeap()->malloc(getHeapIndex(), size);
  }

  static void free(void * ptr) {
    getHeap()->free(getHeapIndex(), ptr);
  }

  static size_t getSize (void * ptr) {
    return getHeap()->getSize (ptr);
  }

  static SuperHeap * getHeap() {
    return InternalHeap::getInstance()._heap; 
  }
  
private:
  SuperHeap * _heap;
};

class InternalHeapAllocator {
public:
  void * malloc (size_t sz) {
    return InternalHeap::getInstance().malloc(sz);
  }
  
  void free (void * ptr) {
    return InternalHeap::getInstance().free(ptr);
  }
};

#endif
