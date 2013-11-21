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

/*
 * @file   source_internalheap.h
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */


#ifndef _SOURCE_INTERNALHEAP_H_
#define _SOURCE_INTERNALHEAP_H_

#include "xdefines.h"
#include "mm.h"

class SourceInternalHeap
{
public:

  SourceInternalHeap()
  { }

  void * initialize(void * startaddr, size_t initSize, size_t metasize) {
    void * ptr;
    char * base;
    size_t size = initSize;
 
    // Call mmap to allocate a shared map.
    //ptr = MM::mmapAllocatePrivate(size+metasize, (void *)xdefines::INTERNAL_HEAP_BASE);
    ptr = MM::mmapAllocatePrivate(size+metasize, (void *)startaddr);
    base = (char *)ptr;

    pthread_mutex_init(&_mutex, NULL);

    // Initialize the following content according the values of xpersist class.
    base = (char *)((intptr_t)ptr + metasize);
    _start      = base;
    _end        = base + size;
    _position  = (char *)_start;
    _remaining = size;
    _magic     = 0xCAFEBABE;
    return ptr; 
  }

  inline void * malloc (size_t sz) {
//    PRINF("inside source internal heap, sz %lx magic %lx at %p\n", sz, _magic, &_magic);
    sanityCheck();

    // We need to page-align size, since we don't want two different
    // threads using the same page.

    // Round up the size to page aligned.
    sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1) / xdefines::PageSize);
    
    lock();
    
    if (_remaining < sz) {
      fprintf (stderr, "Out of memory error: available = %lx, requested = %lx, thread = %d.\n",
               _remaining, sz, (int) pthread_self());
      exit(-1);
    }

    void * p = _position;

    // Increment the bump pointer and drop the amount of memory.
    _remaining -= sz;
    _position += sz;

    unlock();

    //fprintf (stderr, "%d : shareheapmalloc %p with size %x, remaining %x\n", getpid(), p, sz, *_remaining);
    return p;
  }

  inline void * getHeapStart() {
    PRINF("*****XHEAP:_start %p*****\n", _start);
    return (void *)_start;
  }

  inline void * getHeapEnd() {
    return (void *)_end;
  }
  
  inline void * getHeapPosition() {
    return (void *)_position;
  }

  // These should never be used.
  inline void free (void * ptr) { sanityCheck(); }
  inline size_t getSize (void * ptr) { sanityCheck(); return 0; } // FIXME

private:

  void lock() {
    Real::pthread_mutex_lock()(&_mutex);
  }

  void unlock() {
    Real::pthread_mutex_unlock()(&_mutex);
  }

  void sanityCheck() {
    if (_magic != 0xCAFEBABE) {
      fprintf (stderr, "%d : WTF!\n", getpid());
      ::abort();
    }
  }

  /// The start of the heap area.
  volatile char *  _start;

  /// The end of the heap area.
  volatile char *  _end;

  /// Pointer to the current bump pointer.
  char *  _position;

  /// Pointer to the amount of memory remaining.
  size_t  _remaining;

  size_t  _magic;

  // We will use a lock to protect the allocation request from different threads.
  pthread_mutex_t _mutex;
};

#endif
