#if !defined(DOUBLETAKE_XMAPPING_H)
#define DOUBLETAKE_XMAPPING_H

/*
 * @file   xmapping.h
 * @brief  Manage all about mappings, such as changing the protection and unprotection,
 *         managing the relation between shared and private pages.
 *         Adopt from sheiff framework, but there are massive changes.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include "log.hh"
#include "mm.hh"
#include "xdefines.hh"

class xmapping {
public:
  xmapping() : _startaddr(NULL), _startsize(0) {}

  // Initialize the map and corresponding part.
  void initialize(void* startaddr = 0, size_t size = 0, void* heapstart = NULL) {
    REQUIRE(size % xdefines::PageSize == 0, "Wrong size %zx, should be page aligned", size);
    PRINF("xmapping starts at %p, size %zx", startaddr, size);

    // Establish two maps to the backing file.
    // The persistent map is shared.
    _backupMemory = (char*)MM::mmapAllocatePrivate(size);

    // If we specified a start address (globals), copy the contents into the
    // persistent area now because the transient memory mmap call is going
    // to squash it.
    _heapStart = heapstart;

    _userMemory = (char*)startaddr;

    // The transient map is private and optionally fixed at the
    // desired start address.
    _startsize = size;
    _startaddr = (void*)_userMemory;
    _endaddr = (void*)((intptr_t)_userMemory + _startsize);
  }

  // Do nothing
  void finalize() {}

  /// @return the start of the memory region being managed.
  inline char* base() const { return _userMemory; }

  /// @return the size in bytes of the underlying object.
  inline size_t size() const { return _startsize; }

  inline bool checkHeapOverflow(void* end) {
    assert(_heapStart != NULL);

    bool hasOverflow = false;

//		PRINT("xmapping: calling checkHeapIntegrity _heapStart %p end %p\n", _heapStart, end);
    // We only need to check those allocated heap.
    hasOverflow = sentinelmap::getInstance().checkHeapIntegrity(_heapStart, end);
//		PRINT("xmapping: calling checkHeapIntegrity hasOverflow %d\n", hasOverflow);

    return hasOverflow;
  }

  // For the page
  void backup(void* end) {
    size_t sz;

    if(_heapStart) {
      sz = (intptr_t)end - (intptr_t)base();
    } else {
      // Commit all pages
      sz = size();
    }

    // Copy everything to _backupMemory From _userMemory
    memcpy(_backupMemory, _userMemory, sz);
  }

  // How to commit some memory
  void commit(void* start, size_t size) {
    size_t offset = (intptr_t)start - (intptr_t)base();

    void* dest = (void*)((intptr_t)_backupMemory + offset);
    memcpy(dest, start, size);
  }

  // Release all temporary pages.
  void recoverMemory(void* end) {
    size_t sz;

    if(_heapStart) {
      sz = (intptr_t)end - (intptr_t)base();
    } else {
      // Commit all pages
      sz = size();
    }

    // PRINF("Recover memory %p end %p size %lx\n", _userMemory, end, sz);
    memcpy(_userMemory, _backupMemory, sz);
  }

private:
  /// The starting address of the region.
  void* _startaddr;

  void* _heapStart;

  /// The size of the region.
  size_t _startsize;

  /// The starting address of the region.
  void* _endaddr;

  /// The transient (not yet backed) memory.
  char* _userMemory;

  /// The persistent (backed to disk) memory.
  char* _backupMemory;
};

#endif
