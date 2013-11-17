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
 * @file   xheap.h
 * @brief  A basic bump pointer heap, used as a source for other heap layers.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */


#ifndef _XHEAP_H_
#define _XHEAP_H_

#include "xdefines.h"

//template <unsigned long Size>
class xheap 
{
public:

  // It is very very important to put the first page in a separate page since it is shared by different
  // processes or threads. Thus, we don't need to commit this page, which can affect the performance 
  // greately and also can affect the correctness: we don't want different threads 
  // end up getting the same memory from the private mapping. 
  // It is possible that we don't need sanityCheck any more, but we definitely need a lock to avoid the race on "metatdata".
  xheap ()
  {
  }

  void * initialize(size_t startsize, size_t metasize) 
  {
    void * ptr;

    // We must initialize the heap now.
    void * startHeap = (void *)(USER_HEAP_BASE - metasize);

    //fprintf(stderr, "heap size %lx metasize %lx, startHeap %p\n", startsize, metasize, startHeap); 
    ptr = MM::mmapAllocatePrivate(startsize+metasize, (void *)startHeap);

    // Initialize the lock.
    pthread_mutex_init(&_lock, NULL);
  
    // Initialize the following content according the values of xpersist class.
    _start      = (char *)((intptr_t)ptr + metasize);
    _end        = (void *)((intptr_t)_start + startsize);
    _position  = (char *)_start;
    _remaining = startsize;
    _magic     = 0xCAFEBABE;
    fprintf(stderr, "XHEAP:_start %p end is %p\n", _start, _end);
    return ptr;
    // PRDBG("XHEAP:_start at %p end at %p mutexlock %p\n", &_start, &_end, _lock);
  }


  inline void * getHeapStart(void) {
    //PRDBG("*****XHEAP:_start %p*****\n", _start);
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

    //PRFATAL("****xheap malloc %lx, p %p***\n", sz, p);
    //fprintf(stderr, "****THREAD%d: xheap malloc %lx, p %p***\n", getThreadIndex(), sz, p);
    unlock();
    
    // Now we cleanup the corresponding 
    //printf("%d: XHEAP malloc: ptr %p end 0x%lx size %x\n", getpid(),  p, (intptr_t)p + sz,  sz);
    return p;
  }

  // These should never be used.
  inline void free (void * ptr) { sanityCheck(); }
  inline size_t getSize (void * ptr) { sanityCheck(); return 0; } 

private:

  void lock() {
    pthread_mutex_lock(&_lock);
  }

  void unlock() {
    pthread_mutex_unlock(&_lock);
  }

  void sanityCheck (void) {
    if (_magic != 0xCAFEBABE) {
      PRFATAL("Sanitycheck failed for xheap\n");
    }
  }

  /// The start of the heap area.
  void *  _start;

  /// The end of the heap area.
  void *  _end;

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

