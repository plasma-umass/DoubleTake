#if !defined(DOUBLETAKE_INTERNALHEAP_H)
#define DOUBLETAKE_INTERNALHEAP_H

#include <stddef.h>
#include <stdint.h>

#include <new>

#include "log.hh"
#include "objectheader.hh"
#include "sourceinternalheap.hh"
#include "xdefines.hh"
#include "xoneheap.hh"
#include "xpheap.hh"

// From heaplayers
#include "heaps/general/kingsleyheap.h"
#include "wrappers/ansiwrapper.h"

/**
 * @file InternalHeap.h
 * @brief A share heap for internal allocation needs.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *
 */
template <class SourceHeap>
class InternalAdaptHeap : public SourceHeap {

public:
  void * malloc (size_t sz) {

    // We are adding one objectHeader and two "canary" words along the object
    // The layout will be:  objectHeader + "canary" + Object + "canary".
    //PRINF("InternalAdaptHeap before malloc sz %d\n", sz);
    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader) + 2*xdefines::SENTINEL_SIZE);    if (!ptr) {
      return NULL;
    }

    // There is no operations to set sentinals. 
    // We can use check by introducing a flag, however,
    // that can cause a little bit performance problem. 

    // Set the objectHeader. 
    objectHeader * o = new (ptr) objectHeader (sz);
    void * newptr = getPointer(o);

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
class InternalKingsleyStyleHeap :
  public
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
        Kingsley::size2Class,
        Kingsley::class2Size,
        HL::AdaptHeap<HL::SLList, InternalAdaptHeap<SourceHeap> >,
        InternalAdaptHeap<HL::ZoneHeap<SourceHeap, Chunky> > > >
{
};


template <class SourceHeap>
class perheap : public SourceHeap 
{
  //typedef PerThreadHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap, InternalAdaptHeap<SourceHeap>, xdefines::INTERNAL_HEAP_CHUNK> >
  typedef PerThreadHeap<xdefines::NUM_HEAPS, InternalKingsleyStyleHeap<SourceHeap, xdefines::INTERNAL_HEAP_CHUNK> >
  SuperHeap;

public: 
  perheap() {
  }

  void initialize() {
    int  metasize = alignup(sizeof(SuperHeap), xdefines::PageSize);

    // Initialize the SourceHeap before malloc from there.
    char * base = (char *) SourceHeap::initialize((void *)xdefines::INTERNAL_HEAP_BASE, xdefines::INTERNAL_HEAP_SIZE, metasize);
  
    REQUIRE(base != NULL, "Failed to allocated memory for heap metadata");
    PRINF("Internal heap base %p, metasize %xz", base, metasize);
    
    _heap = new (base) SuperHeap;
    
    // Get the heap start and heap end;
    _heapStart = SourceHeap::getHeapStart();
    _heapEnd = SourceHeap::getHeapEnd();
  }

  void * malloc(int heapid, size_t size) {
    return _heap->malloc(heapid, size);
  }

  void free(int heapid, void * ptr) {
    _heap->free(heapid, ptr);
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


class InternalHeap { 

public:

  InternalHeap() {}

  void initialize()
  {
    _heap.initialize();
  }
 
  virtual ~InternalHeap() {}
  
  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here, for use by the
  // signal handler.
  static InternalHeap& getInstance() {
    static char buf[sizeof(InternalHeap)];
    static InternalHeap * theOneTrueObject = new (buf) InternalHeap();
    return *theOneTrueObject;
  }
  
  void * malloc (size_t sz) {
    void * ptr = NULL;
    ptr = _heap.malloc (getThreadIndex(), sz);
  
    REQUIRE(ptr != NULL, "Shareheap is exhausted");
  
    return ptr;
  }
  
  void free (void * ptr) {
    _heap.free (getThreadIndex(), ptr);
  }
  
private:
  perheap<xoneheap<SourceInternalHeap > >  _heap;  
};

#endif
