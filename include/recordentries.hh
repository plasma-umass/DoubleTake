#if !defined(DOUBLETAKE_RECORDENTRIES_H)
#define DOUBLETAKE_RECORDENTRIES_H

/*
 * @file   recordentires.h
 * @brief  Managing record entry for each thread. Since each thread will have different entries,
 There is no need to use lock here at all.
 The basic idea of having pool is
 to reduce unnecessary memory allocation and deallocation operations, similar
 to slab manager of Linux system. However, it is different here.
 There is no memory deallocation for each pool.
 In the same epoch, we keep allocating from
 this pool and udpate correponding counter, updating to next one.
 When epoch ends, we reset the counter to 0 so that we can reuse all
 memory and there is no need to release the memory of recording entries.

 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.hh"
#include "mm.hh"
#include "xdefines.hh"
//Each thread will have a class liked this. Thus, we can avoid 
//memory allocation when we are trying to record a synchronization event.
//The total number of entries for each thread is xdefines::MAX_SYNCEVENT_ENTRIES.
//Thus, there is no need to reclaim it. 
//When an epoch is finished, we will call   
template <class Entry> class RecordEntries {
public:
  RecordEntries() {}

  void initialize(int entries) {
    void* ptr;

    _size = alignup(entries * sizeof(Entry), xdefines::PageSize);
    ptr = MM::mmapAllocatePrivate(_size);
    if(ptr == NULL) {
      PRWRN("%d fail to allocate sync event pool entries : %s\n", getpid(), strerror(errno));
      ::abort();
    }

    //  PRINF("recordentries.h::initialize at _cur at %p. memory from %p to 0x%lx\n", &_cur, ptr,
    // (((unsigned long)ptr) + _size));
    // start to initialize it.
    _start = (Entry*)ptr;
    _cur = 0;
    _total = entries;
    _iter = 0;
    return;
  }

  Entry* alloc() {
    Entry* entry = NULL;
	//	PRINF("allocEntry, _cur %ld\n", _cur);
    if(_cur < _total) {
      entry = (Entry*)&_start[_cur];
      _cur++;
    } else {
      // There are no enough entries now; re-allocate new entries now.
      PRWRN("Not enough entries, now _cur %lu, _total %lu at %p!!!\n", _cur, _total, &_cur);
      ::abort();
    }
    return entry;
  }

  void cleanup() {
    _iter = 0;
    _cur = 0;
  }

  void prepareRollback() { _iter = 0; }

  void prepareIteration() { _iter = 0; }

  inline Entry* getEntry(size_t index) { return &_start[index]; }

  Entry* nextIterEntry() {
    _iter++;
    if(_iter < _cur) {
      return getEntry(_iter);
    } else {
      return NULL;
    }
  }

	void advanceEntry() {
    _iter++;
	}

  Entry* retrieveIterEntry() {
    Entry* entry = NULL;
    if(_iter < _cur) {
      entry = getEntry(_iter);
      _iter++;
    }
    return entry;
  }

  // No change on iteration entry.
  Entry* getEntry() {
    Entry* entry = NULL;
		PRINF("getEntry: _iter %ld, _cur %ld\n", _iter, _cur);
    if(_iter < _cur) {
      entry = getEntry(_iter);
    }
    return entry;
  }

  // Only called in the replay
  Entry* firstIterEntry() { return &_start[_iter]; }

  size_t getEntriesNumb() { return _cur; }

private:
  Entry* _start;
  size_t _total;
  size_t _cur;
  size_t _size;
  size_t _iter;
};

#endif
