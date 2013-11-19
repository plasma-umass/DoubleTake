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

//#include "xpersist.h"
#include "xdefines.h"
#include "xmapping.h"

//template <unsigned long Size>
class xheap : public xmapping
{
  typedef xmapping parent;

public:

  // It is very very important to put the first page in a separate
  // page since it is shared by different processes or threads. Thus,
  // we don't need to commit this page, which can affect the
  // performance greately and also can affect the correctness: we
  // don't want different threads end up getting the same memory from
  // the private mapping.  It is possible that we don't need
  // sanityCheck any more, but we definitely need a lock to avoid the
  // race on "metadata".

  void * initialize(void * startaddr, size_t startsize, size_t metasize) 
  {
    void * ptr;
    
    // Initialize corresponding heapowner
    _unitSize = xdefines::USER_HEAP_CHUNK + xdefines::PageSize;
    int units = startsize/_unitSize;
    int ownermapSize = alignup(units * sizeof(int), xdefines::PageSize);

    // We must initialize the heap now.
    void * startHeap
      = (void *)((unsigned long) xdefines::USER_HEAP_BASE - (unsigned long) metasize);
    
    //    DEBUG("heap size %lx metasize %lx, startHeap %p\n", startsize, metasize, startHeap); 
    ptr = MM::mmapAllocatePrivate(startsize+metasize+ownermapSize, startHeap);

    // Initialize the lock.
    pthread_mutex_init(&_lock, NULL);

    // Initialize the following content according the values of xpersist class.
    _start      = (char *)((intptr_t)ptr + metasize);
    _end        = (char *)((intptr_t)_start + startsize);
    _position  = (char *)_start;
    _remaining = startsize;
    _magic     = 0xCAFEBABE;
    _owners     = (int *)_end;
    
    // Register this heap so that they can be recoved later.
    parent::initialize(ptr, startsize+metasize, (void*)_start);

    // Initialize the freelist.
    freelist::getInstance().initialize();

    return (void *)ptr;	
    DEBUG("XHEAP %p - %p, position: %p, remaining: %#lx", _start, _end, _position, _remaining);
  }

  /*
   * We don't need to backup the metadata for each allocation.
   * We will do this for each transaction.
   * Also, we can recover it if we want to do rollback, only for single thread program!!!FIXME.
   * However, this approach is not good for multithreaded program since
   * a transaction may acquire memory sever times and they are interleaved
   * as other threads. We CANNOT do a deterministic replay because of this.
   * We may endup using a list to keep those newly allocated objects everytime.
   * In the rollback phase, we only reuse those previous allocated objects. 
   */ 
  /// We will save those pointers to the backup ones.
  inline void saveHeapMetadata(void) {
    _positionBackup = _position;
    _remainingBackup = _remaining;
    DEBUG("save heap metadata, _position %p remaining %#lx\n", _position, _remaining);
  }

  /// We will overlap the metadata with the saved ones
  /// when we need to backup
  inline void recoverHeapMetadata(void) {
    _position = _positionBackup;
    _remaining = _remainingBackup;
    DEBUG("in recover, now _position %p remaining 0x%lx\n", _position, _remaining);
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
    //DEBUG("GetHeapPosition %p\n", _position);
	  return  _position;
  }

  inline void registerOwner(void * start, size_t sz) {
    int threadindex = getThreadIndex();
    int units = sz/_unitSize;

    for(int i = 0; i < units; i++) {
      _owners[i] = threadindex;
    }
  }

  inline size_t findSize(size_t sz) {
    int units = sz/_unitSize;

    if(sz%_unitSize != 0) {
      units++;
    } 
    return units * _unitSize;
  }

  inline size_t getMapIndex(void * addr) {
    return ((intptr_t)addr - (intptr_t)_start)/_unitSize;
  }

  inline int getOwner(void * addr) {
    if(addr > _position) {
      return -1;
    }
    else {
      return _owners[getMapIndex(addr)];
    }
  }

  // We need to page-aligned size, we don't want that
  // two different threads are using the same page here.
  inline void * malloc (size_t sz) {
    // Roud up the size to page aligned.
	  sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1) / xdefines::PageSize);
      
    // Find smallest sz which is multiple of _unitSize
    sz = findSize(sz);

    void * p;

	  lock();

    if (_remaining < sz) { 
      fprintf (stderr, "Fatal error: out of memory for heap.\n");
      fprintf (stderr, "Fatal error: remaining %lx sz %lx\n", _remaining, sz);
      exit (-1);
    }

    _remaining -= sz;
    p = _position;
    // Increment the bump pointer and drop the amount of memory.
    _position += sz;

    registerOwner(p, sz);

	  unlock();
    
#if defined (DETECT_OVERFLOW) || defined (DETECT_MEMORY_LEAKAGE)
    // We must cleanup corresponding bitmap 
   sentinelmap::getInstance().cleanup(p, sz);
#endif
    // Now we cleanup the corresponding 
    //printf("%d: XHEAP malloc: ptr %p end 0x%lx size %x. Heappoition %p\n", getpid(),  p, (intptr_t)p + sz,  sz, _position);
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

  void sanityCheck() {
    REQUIRE(_magic == 0xCAFEBABE, "Sanity check failed for xheap");
  }

  /// The start of the heap area.
  volatile char *  _start;

  /// The end of the heap area.
  volatile char *  _end;

  /// Pointer to the current bump pointer.
  char *  _position;

  /// Pointer to the amount of memory remaining.
  size_t  _remaining;

  size_t _unitSize; 

  /// For single thread program, we are simply adding
  /// two backup pointers to backup the metadata.
  char *  _positionBackup;

  /// Pointer to the amount of memory remaining.
  size_t  _remainingBackup;


  int    * _owners;

  /// A magic number, used for sanity checking only.
  size_t  _magic;

  // This lock guards allocation requests from different threads.
  pthread_mutex_t _lock;
};

#endif
