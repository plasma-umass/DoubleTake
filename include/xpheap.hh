#if !defined(DOUBLETAKE_XPHEAP_H)
#define DOUBLETAKE_XPHEAP_H

/*
 * @file   xpheap.h
 * @brief  A heap optimized to reduce the likelihood of false sharing.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <stddef.h>

#include <new>

#include "log.hh"
#include "objectheader.hh"
#include "sentinelmap.hh"
#include "xdefines.hh"

// Include all of heaplayers
#include "heaplayers.h"

template <class SourceHeap>
class AdaptAppHeap : public SourceHeap {

public:
  void * malloc (size_t sz) {

    // We are adding one objectHeader and two "canary" words along the object
    // The layout will be:  objectHeader + "canary" + Object + "canary".
#if defined (DETECT_OVERFLOW) || defined (DETECT_MEMORY_LEAKS) 
    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader) + 2*xdefines::SENTINEL_SIZE);
#else
    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader));
#endif
    if (!ptr) {
      return NULL;
    }
  
    // Set the objectHeader. 
    objectHeader * o = new (ptr) objectHeader (sz);
    void * newptr = getPointer(o);

    // Now we are adding two sentinels and mark them on the shared bitmap.
    //PRINF("AdaptAppHeap malloc sz %d ptr %p nextp %lx\n", sz, ptr, (intptr_t)ptr + sz + sizeof(objectHeader) + 2 * xdefines::SENTINEL_SIZE);
#if defined (DETECT_OVERFLOW) || defined (DETECT_MEMORY_LEAKS) 
    sentinelmap::getInstance().setupSentinels(newptr, sz); 
#endif

    assert (getSize(newptr) == sz);

  //  PRINF("NEWSOURCEHEAP: sz is %x - %d, newptr %p\n", sz, sz, newptr); 
    return newptr;
  }

  void free (void * ptr) {
    SourceHeap::free ((void *) getObject(ptr));
  }

  size_t getSize (void * ptr) {
    objectHeader * o = getObject(ptr);
    size_t sz = o->getSize();
    REQUIRE(sz != 0, "Object size cannot be zero");
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
class KingsleyStyleHeap :
  public 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, AdaptAppHeap<SourceHeap> >,
		    AdaptAppHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
{
private:

  typedef 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::SLList, AdaptAppHeap<SourceHeap> >,
		    AdaptAppHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
  SuperHeap;

public:
  KingsleyStyleHeap() {
  }

private:
  // We want that a single heap's metadata are on different page
  // to avoid conflicts on one page
//  char buf[4096 - (sizeof(SuperHeap) % 4096)];
};

// Different processes will have a different heap.
//class PerThreadHeap : public TheHeapType {
template <int NumHeaps,
	  class TheHeapType>
class PerThreadHeap {
public:
  PerThreadHeap()
  {
  //  PRINF("TheHeapType size is %ld\n", sizeof(TheHeapType)); 
  }

  void * malloc (int ind, size_t sz)
  {
//    PRINF("PerThreadheap malloc ind %d sz %d _heap[ind] %p\n", ind, sz, &_heap[ind]);
    // Try to get memory from the local heap first.
    void * ptr = _heap[ind].malloc (sz);
    return ptr;
  }
 
  // Here, we will give one block of memory back to the originated process related heap. 
  void free (int ind, void * ptr)
  { 
    REQUIRE(ind < NumHeaps, "Invalid free status");
    _heap[ind].free (ptr);
    //PRINF("now first word is %lx\n", *((unsigned long*)ptr));
  }

  // For getSize, it doesn't matter which heap is used 
  // since they are the same
  size_t getSize (void * ptr) {
    return _heap[0].getSize (ptr);
  }

private:
  TheHeapType _heap[NumHeaps];
};

// Protect heap 
template <class SourceHeap>
class xpheap : public SourceHeap 
{
  typedef PerThreadHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap, xdefines::USER_HEAP_CHUNK> >
  SuperHeap;
  //typedef PerThreadHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap, AdaptAppHeap<SourceHeap>, xdefines::USER_HEAP_CHUNK> >

public: 
  xpheap() {
  }

  void * initialize(void * start, size_t heapsize) {

    int  metasize = alignup(sizeof(SuperHeap), xdefines::PageSize);

    // Initialize the SourceHeap before malloc from there.
    char * base = (char *)SourceHeap::initialize(start, heapsize, metasize);
    REQUIRE(base != NULL, "Failed to allocate memory for heap metadata");

    _heap = new (base) SuperHeap;
    //PRINF("xpheap calling sourceHeap::malloc size %lx base %p metasize %lx\n", metasize, base, metasize);
  
    // Get the heap start and heap end;
    _heapStart = SourceHeap::getHeapStart();
    _heapEnd = SourceHeap::getHeapEnd();

    // Sanity check related information
#if defined (DETECT_OVERFLOW) || defined (DETECT_MEMORY_LEAKS) 
    void * sentinelmapStart;
    size_t sentinelmapSize;
    sentinelmapStart = _heapStart;
    sentinelmapSize = xdefines::USER_HEAP_SIZE;
    //PRINF("INITIAT: sentinelmapStart %p _heapStart %p original size %lx\n", sentinelmapStart, _heapStart, xdefines::USER_HEAP_SIZE);
    // Initialize bitmap
    sentinelmap::getInstance().initialize(sentinelmapStart, sentinelmapSize);
#endif
 
    return (void *)_heapStart;
   // base = (char *)malloc(0, 4); 
  }


  // Performing the actual sanity check.
  bool checkHeapOverflow() {
    void * heapEnd =(void *)SourceHeap::getHeapPosition();
    return SourceHeap::checkHeapOverflow(heapEnd);
  }

  void recoverMemory() {
    void * heapEnd =(void *)SourceHeap::getHeapPosition();
    //PRINF("recoverMemory, heapEnd %p\n", heapEnd);
    SourceHeap::recoverMemory(heapEnd);
  }

  void backup() {
    void * heapEnd =(void *)SourceHeap::getHeapPosition();
    return SourceHeap::backup(heapEnd);
  }

  void * getHeapEnd() {
    return (void *)SourceHeap::getHeapPosition();
  }
 
  void * malloc(size_t size) {
    //printf("malloc in xpheap with size %d\n", size);
    return _heap->malloc(getThreadIndex(), size);
  }

  void free(void * ptr) {
    int tid = getThreadIndex();

  #ifndef DETECT_USAGE_AFTER_FREE
    _heap->free(tid, ptr);
  #else
    size_t size = getSize(ptr); 
    // Adding this to the quarantine list
    if(addThreadQuarantineList(ptr, size) == false) {
      // If an object is too large, we simply freed this object.
      _heap->free(tid, ptr);
    }
  #endif
  }

  void realfree(void * ptr) {
    _heap->free(getThreadIndex(), ptr);
  }

  size_t getSize (void * ptr) {
    return _heap->getSize (ptr);
  }

  bool inRange(void * addr) {
    return ((addr >= _heapStart) && (addr <= _heapEnd)) ? true : false;
  }
 
private:
  SuperHeap * _heap;
  void * _heapStart;
  void * _heapEnd; 
};

#endif

