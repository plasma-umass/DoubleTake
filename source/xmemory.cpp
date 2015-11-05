/*
 * @file   xmemory.cpp
 * @brief  Memory management for all.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xmemory.hh"

#include "internalheap.hh"
#include "mm.hh"
#include "xoneheap.hh"
#include "xpheap.hh"
#include "xrun.hh"

xpheap<xoneheap<xheap>> xmemory::_pheap;

void xmemory::initialize() {
  _selfmap.initialize();

  // Call _pheap so that xheap.h can be initialized at first and
  // then can work normally.
  _heapBegin =
    (intptr_t)_pheap.initialize((void*)xdefines::USER_HEAP_BASE, xdefines::USER_HEAP_SIZE);
  _heapEnd = _heapBegin + xdefines::USER_HEAP_SIZE;

  _globals.initialize(_selfmap);
}

void xmemory::finalize() {
  _globals.finalize();
  _pheap.finalize();
}


// This function is called inside the segmentation fault handler
// So we must utilize the "context" to achieve our target
void xmemory::handleSegFault() {
//  PRINT("Returning from segmentation fault error\n");

#ifdef MYDEBUG
	// Adding a watchpoint and rollback immediately
	watchpoint::getInstance().addWatchpoint((void *)0x101003010, sizeof(void *), OBJECT_TYPE_WATCHONLY, NULL, 0);
#endif
  // Check whether the segmentation fault is called by buffer overflow.
  //if(xmemory::getInstance().checkHeapOverflow()) {
    // Now we can roll back
  //  PRINF("\n\nOVERFLOW causes segmentation fault!!!! ROLLING BACK\n\n\n");
  //  PRINT("\n\nOVERFLOW causes segmentation fault!!!! ROLLING BACK\n\n\n");
  //}
    
	//xrun::getInstance().rollback();
}

void xmemory::realfree(void* ptr) { _pheap.realfree(ptr); }

void* InternalHeapAllocator::malloc(size_t sz) { return InternalHeap::getInstance().malloc(sz); }

void InternalHeapAllocator::free(void* ptr) { return InternalHeap::getInstance().free(ptr); }

void* InternalHeapAllocator::allocate(size_t sz) { return InternalHeap::getInstance().malloc(sz); }

void InternalHeapAllocator::deallocate(void* ptr) { return InternalHeap::getInstance().free(ptr); }
