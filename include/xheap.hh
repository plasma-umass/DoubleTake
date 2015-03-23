#if !defined(DOUBLETAKE_XHEAP_H)
#define DOUBLETAKE_XHEAP_H

/*
 * @file   xheap.h
 * @brief  A basic bump pointer heap, used as a source for other heap layers.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.hh"
#include "mm.hh"
#include "xdefines.hh"
#include "xmapping.hh"

// template <unsigned long Size>
class xheap : public xmapping {
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

  void* initialize(void*, size_t startsize, size_t metasize) {
    void* ptr;

    // We must initialize the heap now.
    void* startHeap = (void*)((unsigned long)xdefines::USER_HEAP_BASE - (unsigned long)metasize);

    //    PRINF("heap size %lx metasize %lx, startHeap %p\n", startsize, metasize, startHeap);
    ptr = MM::mmapAllocatePrivate(startsize + metasize, startHeap);

    // Initialize the lock.
    pthread_mutex_init(&_lock, NULL);

    // Initialize the following content according the values of xpersist class.
    _start = (char*)((intptr_t)ptr + metasize);
    _end = (char*)((intptr_t)_start + startsize);
    _position = (char*)_start;
    _remaining = startsize;
    _magic = 0xCAFEBABE;

    // Register this heap so that they can be recoved later.
    parent::initialize(ptr, startsize + metasize, (void*)_start);

    PRINF("XHEAP %p - %p, position: %p, remaining: %#lx", _start, _end, _position, _remaining);

    return (void*)ptr;
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
  inline void saveHeapMetadata() {
    _positionBackup = _position;
    _remainingBackup = _remaining;
    PRINF("save heap metadata, _position %p remaining %#lx\n", _position, _remaining);
  }

  /// We will overlap the metadata with the saved ones
  /// when we need to backup
  inline void recoverHeapMetadata() {
    _position = _positionBackup;
    _remaining = _remainingBackup;
    PRINF("in recover, now _position %p remaining 0x%lx\n", _position, _remaining);
  }

  inline void* getHeapStart() { return (void*)_start; }

  inline void* getHeapEnd() { return (void*)_end; }

  // Get current heap position
  // We only need to do the sanity check until current position.
  inline void* getHeapPosition() {
    // PRINF("GetHeapPosition %p\n", _position);
    return _position;
  }

  // We need to page-aligned size, we don't want that
  // two different threads are using the same page here.
  inline void* malloc(size_t sz) {
    // Roud up the size to page aligned.
    sz = xdefines::PageSize * ((sz + xdefines::PageSize - 1) / xdefines::PageSize);

    void* p;

    lock();

    if(_remaining < sz) {
      fprintf(stderr, "Fatal error: out of memory for heap.\n");
      fprintf(stderr, "Fatal error: remaining %lx sz %lx\n", _remaining, sz);
      exit(-1);
    }

    _remaining -= sz;
    p = _position;
    // Increment the bump pointer and drop the amount of memory.
    _position += sz;

    unlock();

#if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKS)
    // We must cleanup corresponding bitmap
    sentinelmap::getInstance().cleanup(p, sz);
#endif
    // Now we cleanup the corresponding
    // printf("%d: XHEAP malloc: ptr %p end 0x%lx size %x. Heappoition %p\n", getpid(),  p,
    // (intptr_t)p + sz,  sz, _position);
    return p;
  }

  // These should never be used.
  inline void free(void*) { sanityCheck(); abort(); }
  inline size_t getSize(void*) {
    sanityCheck();
    abort();
    return 0;
  }

private:
  void lock() { pthread_mutex_lock(&_lock); }

  void unlock() { pthread_mutex_unlock(&_lock); }

  void sanityCheck() { REQUIRE(_magic == 0xCAFEBABE, "Sanity check failed for xheap"); }

  /// The start of the heap area.
  volatile char* _start;

  /// The end of the heap area.
  volatile char* _end;

  /// Pointer to the current bump pointer.
  char* _position;

  /// Pointer to the amount of memory remaining.
  size_t _remaining;

  /// For single thread program, we are simply adding
  /// two backup pointers to backup the metadata.
  char* _positionBackup;

  /// Pointer to the amount of memory remaining.
  size_t _remainingBackup;

  /// A magic number, used for sanity checking only.
  size_t _magic;

  // This lock guards allocation requests from different threads.
  pthread_mutex_t _lock;
};

#endif
