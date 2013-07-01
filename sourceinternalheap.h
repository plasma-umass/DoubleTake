// -*- C++ -*-

/*
  Copyright (c) 2007-12, University of Massachusetts Amherst.

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
 * @file   souceinternalheap.h
 * @brief  The source of internal heap.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */


#ifndef _SOURCE_INTERNAL_HEAP_H_
#define _SOURCE_INTERNAL_HEAP_H_

#include "xdefines.h"
#include "mm.h"

class SourceInternalHeap 
{

public:
  SourceInternalHeap ()
  {
  }

  void initialize(void * startaddr, size_t startsize) 
  {
    // We must initialize the heap now. 
	  char * ptr = (char *)MM::mmapAllocatePrivate(startsize);

	  // Put all "heap metadata" in this page.
    _start      = (char *)ptr;;
    _end        = (char *)_start + startsize;
    _position   = (char *)ptr;
    _remaining  = startsize;
    WRAP(pthread_mutex_init)(&_lock, NULL);
	
    _magic     = 0xCAFEBABE;
  }

  inline void * getHeapStart(void) {
    return (void *)_start;
  }

  inline void * getHeapEnd(void) {
    return (void *)_end;
  }

  // Get current heap position
  // We only need to do the sanity check until current position.
  inline void * getHeapPosition(void) {
	  return  _position;
  }

  // We need to page-aligned size, we don't want that
  // two different threads are using the same page here.
  inline void * malloc (size_t sz) {
    sanityCheck();

    // Roud up the size to page aligned.
	  sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1) / xdefines::PageSize);

	  lock();

    if (_remaining < sz) { 
      fprintf (stderr, "Fatal error: out of memory for heap.\n");
      fprintf (stderr, "Fatal error: remaining %lx sz %lx\n", _remaining, sz);
      exit (-1);
    }

    void * p = _position;

    // Increment the bump pointer and drop the amount of memory.
    _remaining -= sz;
    _position += sz;

	  unlock();
    
    return p;
  }

  // These should never be used.
  inline void free (void * ptr) { sanityCheck(); }
  inline size_t getSize (void * ptr) { sanityCheck(); return 0; } 

private:

  void lock() {
    WRAP(pthread_mutex_lock)(&_lock);
  }

  void unlock() {
    WRAP(pthread_mutex_unlock)(&_lock);
  }

  void sanityCheck (void) {
    if (_magic != 0xCAFEBABE) {
      PRFATAL("Sanitycheck failed for SourceInternalHeap\n");
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

  /// A magic number, used for sanity checking only.
  size_t  _magic;

  // This lock guards allocation requests from different threads.
  pthread_mutex_t _lock;
};

#endif
