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

xpheap<xoneheap<xheap > > xmemory::_pheap;
 
// This function is called inside the segmentation fault handler
// So we must utilize the "context" to achieve our target 
void xmemory::handleSegFault()
{
#ifdef DETECT_OVERFLOW
  PRINF("Returning from segmentation fault error\n"); 
  // Check whether the segmentation fault is called by buffer overflow.
  if(xmemory::getInstance().checkHeapOverflow()) {
    // Now we can roll back 
    PRINF("\n\nOVERFLOW causes segmenation fault!!!! ROLLING BACK\n\n\n");
    
    xrun::getInstance().rollback();
  }  
  else {
    PRINF("\n\nNO overflow in segmenation fault, ROLLING BACK and stop\n\n\n");
    // We may rely on gdb to find out this problem.
    // We do not need to install watchpoints at all.
    // But we can rollback everything and hault there
    // so gdb can be used to attach to this process.
    xrun::getInstance().rollbackandstop();
  }
#else 
  xrun::getInstance().rollbackandstop();
#endif
}

void xmemory::realfree(void * ptr) {
  _pheap.realfree(ptr);
}

void * InternalHeapAllocator::malloc (size_t sz) {
  return InternalHeap::getInstance().malloc(sz);
}

void InternalHeapAllocator::free (void * ptr) {
  return InternalHeap::getInstance().free(ptr);
}

void * InternalHeapAllocator::allocate(size_t sz) {
  return InternalHeap::getInstance().malloc(sz);
}

void InternalHeapAllocator::deallocate (void * ptr) {
  return InternalHeap::getInstance().free(ptr);
}


