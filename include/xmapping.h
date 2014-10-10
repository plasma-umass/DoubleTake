#if !defined(DOUBLETAKE_XMAPPING_H)
#define DOUBLETAKE_XMAPPING_H

/*
 * @file   xmapping.h
 * @brief  Manage all about mappings, such as changing the protection and unprotection, 
 *         managing the relation between shared and private pages.
 *         Adopt from sheiff framework, but there are massive changes.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <set>
#include <list>
#include <vector>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <algorithm> //sort
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#include "xdefines.h"

#include "mm.h"
#include "sentinelmap.h"

using namespace std;

class xmapping {
public:

  xmapping() 
    : _startaddr (NULL),
      _startsize (0)
  {

  }
  
  virtual ~xmapping() {
  }

  // Initialize the map and corresponding part.
  void initialize(void * startaddr = 0, size_t size = 0, void * heapstart = NULL)
  {
    REQUIRE(size % xdefines::PageSize == 0, "Wrong size %lx, should be page aligned", size);
    PRINF("xmapping starts at %p, size %lx", startaddr, size);
    
    // Establish two maps to the backing file.
    // The persistent map is shared.
    _backupMemory = (char *) MM::mmapAllocatePrivate(size);

    // If we specified a start address (globals), copy the contents into the
    // persistent area now because the transient memory mmap call is going
    // to squash it.
    _heapStart = heapstart; 

    _userMemory = (char *)startaddr;

    // The transient map is private and optionally fixed at the
    // desired start address.
    _startsize = size;
    _startaddr = (void *)_userMemory;
    _endaddr = (void *)((intptr_t)_userMemory + _startsize);

    // bitmap bits
#ifndef NDEBUG
   //fprintf (stderr, "transient = %p, size = %lx, _startaddr %p, _endaddr %p\n", _userMemory, _startsize, _startaddr, _endaddr);
#endif

#ifdef SSE_SUPPORT	
    // A string of one bits.
    allones = _mm_setzero_si128();
    allones = _mm_cmpeq_epi32(allones, allones); 
#endif
  }

  // Do nothing 
  void finalize() {
  }

  /// @return the start of the memory region being managed.
  inline char * base() const {
    return _userMemory;
  }

  /// @return the size in bytes of the underlying object.
  inline size_t size() const {
    return _startsize;
  }

#ifdef DETECT_OVERFLOW
  inline bool checkHeapOverflow(void * end) {
    assert(_heapStart != NULL);
 
    bool hasOverflow = false;

    // FIXME: we only need to check those allocated heap.
    hasOverflow = sentinelmap::getInstance().checkHeapIntegrity
                  (_heapStart, end);

    return hasOverflow;
  }
#endif

  // For the page
  void backup(void * end) {
    size_t sz;

    if(_heapStart) {
      sz = (intptr_t)end - (intptr_t)base();
    }
    else {
      // Commit all pages
      sz = size(); 
    }

    // Copy everything to _backupMemory From _userMemory
    customMemcpy(_backupMemory, _userMemory, sz);
  }

  // How to commit some memory
  void commit(void * start, size_t size) {
    size_t offset = (intptr_t)start - (intptr_t)base();
 
    void * dest = (void *)((intptr_t)_backupMemory + offset);
    customMemcpy(dest, start, size);
  }

  // Release all temporary pages.
  void recoverMemory(void *end) {
    size_t sz;

    if(_heapStart) {
      sz = (intptr_t)end - (intptr_t)base();
    }
    else {
      // Commit all pages
      sz = size();
    }

    //PRINF("Recover memory %p end %p size %lx\n", _userMemory, end, sz);
    customMemcpy(_userMemory, _backupMemory, sz);
  }

  void customMemcpy(void * dest, void * src, size_t len) {
#ifdef SSE_SUPPORT
#define PREFETCH "prefetchnta"
    char * from = (char *)src;
    char * to = (char *)dest;

     void *retval;
     size_t i;
     retval = to;
     i = len >> 6; /* len/64 */
//     i = 64; /* len/64 */
  __asm__ __volatile__ (
             PREFETCH" (%0)\n"
             PREFETCH" 64(%0)\n"
             PREFETCH" 128(%0)\n"
             PREFETCH" 192(%0)\n"
             PREFETCH" 256(%0)\n"
              : : "r" (from) );
     /*
        Only if SRC is aligned on 16-byte boundary.
        It allows to use movaps instead of movups, which required data
        to be aligned or a general-protection exception (#GP) is generated.
     */
    for(; i>0; i--)
    {
         __asm__ __volatile__ (
         PREFETCH" 320(%0)\n"
         "movaps (%0), %%xmm0\n"
         "movaps 16(%0), %%xmm1\n"
         "movaps 32(%0), %%xmm2\n"
         "movaps 48(%0), %%xmm3\n"
         "movntps %%xmm0, (%1)\n"
         "movntps %%xmm1, 16(%1)\n"
         "movntps %%xmm2, 32(%1)\n"
         "movntps %%xmm3, 48(%1)\n"
         :: "r" (from), "r" (to) : "memory");
         from+=64;
         to+=64;
    }
    __asm__ __volatile__ ("sfence":::"memory");

//    #error "need support for memcpy"
#else
    memcpy(dest, src, len);
#endif
  }

private:

  /// The starting address of the region.
  void * _startaddr;

  void * _heapStart;

  /// The size of the region.
  size_t _startsize;

  /// The starting address of the region.
  void * _endaddr;

  /// The transient (not yet backed) memory.
  char * _userMemory;

  /// The persistent (backed to disk) memory.
  char * _backupMemory;

  /// Should we use the same framework as bitmap 
#ifdef SSE_SUPPORT
  // A string of one bits, only useful when we are having SSE_SUPPORT.
  __m128i allones;
#endif

};

#endif
