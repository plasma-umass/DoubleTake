// -*- C++ -*-

/*
  Copyright (C) 2011 University of Massachusetts Amherst.

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
 * @file   xmapping.h
 * @brief  Manage all about mappings, such as changing the protection and unprotection, 
 *         managing the relation between shared and private pages.
 *         Adopt from sheiff framework, but there are massive changes.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 


#ifndef _XMAPPING_H_
#define _XMAPPING_H_

#include <set>
#include <list>
#include <vector>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#include "atomic.h"
#include "ansiwrapper.h"
#include "freelistheap.h"

#include "stlallocator.h"
#include "privateheap.h"
#include "xplock.h"
#include "xdefines.h"
#include "xpageentry.h"

#include "mm.h"
#include "sanitycheck.h"

class xmapping {
public:
  typedef std::pair<int, struct pageinfo *> dirtyPagePair;

  // The objects are pairs, mapping void * pointers to sizes.
  typedef HL::STLAllocator<dirtyPagePair, PrivateHeap> dirtyPageTypeAllocator;

  typedef std::less<int> dirtyPageComparator;

  /// A map of pointers to objects and their allocated sizes.
  typedef std::map<int, struct pageinfo *, dirtyPageComparator, dirtyPageTypeAllocator> dirtyListType;
  
#if 0
  typedef HL::STLAllocator<int pageNo, struct pageinfo *, PrivateHeap> dirtyPageTypeAllocator;

  /// A map of pointers to objects and their allocated sizes.
  typedef std::map<int pageNo, struct pageinfo *, dirtyPageTypeAllocator> dirtyListType;
#endif

  xmapping() 
    : _startaddr (NULL),
      _startsize (0)
  {

  }
  
  virtual ~xmapping (void) {
  }

  // Initialize the map and corresponding part.
  void initialize(void * startaddr = 0, size_t size = 0)
  {
    // Setting the scope of this mapping
    _startsize = size;
    _startaddr = startaddr;
 
    // Check whether the size is valid, should be page aligned.
    if(size % xdefines::PageSize != 0) {
      PRFATAL("Wrong size %lx, should be page aligned.\n", size);
    }

    // Get a temporary file name (which had better not be NFS-mounted...).
    char _backingFname[L_tmpnam];
    sprintf (_backingFname, "/tmp/stopgapMXXXXXX");
    _backingFd = mkstemp (_backingFname);
	  if (_backingFd == -1) {
      fprintf (stderr, "Failed to make persistent file.\n");
      ::abort();
	  }
    
	  // Set the files to the sizes of the desired object.
    if(ftruncate(_backingFd, size)) { 
      fprintf (stderr, "Mysterious error with ftruncate.\n");
      ::abort();
    }
    
	  // Get rid of the files when we exit.
    unlink (_backingFname);

    //
    // Establish two maps to the backing file.
    //
    // The persistent map is shared.
    _persistentMemory = 
      (char *) MM::mmapAllocateShared(size, _backingFd);

    // If we specified a start address (globals), copy the contents into the
    // persistent area now because the transient memory map is going
    // to squash it.
    if (_startaddr) {
      memcpy (_persistentMemory, _startaddr, _startsize);
	    _isHeap = false;
    }
  	else {
	    _isHeap = true;
	  }

    // The transient map is private and optionally fixed at the
    // desired start address.
	  // In order to get the same copy with _persistentMemory for those constructor stuff,
    // we will set to MAP_PRIVATE at first, then memory protection will be opened in initialize().
    _transientMemory = 
      (char *) MM::mmapAllocateShared(_startsize, _backingFd, _startaddr);
 

    _startaddr = (void *)_transientMemory;
    _endaddr = (void *)((intptr_t)_transientMemory + _startsize);
#ifndef NDEBUG
    fprintf (stderr, "transient = %p, persistent = %p, size = %lx, _startaddr %p, _endaddr %p\n", _transientMemory, _persistentMemory, _startsize, _startaddr, _endaddr);
#endif

#ifdef SSE_SUPPORT	
    // A string of one bits.
    allones = _mm_setzero_si128();
    allones = _mm_cmpeq_epi32(allones, allones); 
#endif
	  _dirtiedPagesList.clear();
  }

  void finalize(void) {
#if 0
    close (_backingFd);
    // Unmap everything.
    munmap (_transientMemory, size());
    munmap (_persistentMemory, size());
    close (_versionsFd);
#endif
  }

  /* The following two functions are working directly on the shared mapping 
   * by given private mapping address.
   * Normally, this is not good to do this. But it is a way to initialize the 
   * mutex locks.
   */
  void sharemem_write_word(void * addr, unsigned long val) {
    unsigned long offset = (intptr_t)addr - (intptr_t)base();
    *((unsigned long *)((intptr_t)_persistentMemory + offset)) = val;
    return;
  }

  unsigned long sharemem_read_word(void * addr) {
    unsigned long offset = (intptr_t)addr - (intptr_t)base();
    return *((unsigned long *)((intptr_t)_persistentMemory + offset));
  }
  
  /**
   * We need to change mapping for the transient mapping,
   * thus, all following functions are working on _backingFd and 
   * all are working on specified address .
   */
  void * changeMappingToShared(int protInfo, void * start, size_t sz) {
    int  offset = (intptr_t)start - (intptr_t)base();
    return changeMapping(true, protInfo, start, sz, offset);
  }

  void * changeMappingToPrivate(int protInfo, void * start, size_t sz) {
    int  offset = (intptr_t)start - (intptr_t)base();
    return changeMapping(false, protInfo, start, sz, offset);
  }
  
  void * changeMapping (bool isShared, int protInfo, void * start,
            size_t sz, int offset)
  {
    int sharedInfo = isShared ? MAP_SHARED : MAP_PRIVATE;
    sharedInfo     |= MAP_FIXED ;
   
    return mmap (start, sz, protInfo,
                 sharedInfo, _backingFd, offset);
  }
  
  void* mmapRdPrivate(void * start, unsigned long size) {
    void * ptr;
    // Map to readonly private area.
    ptr = changeMappingToPrivate(PROT_READ, start, size); 
    if(ptr == MAP_FAILED) {
      fprintf(stderr, "Weird, %d can't map to read and private area\n", getpid());
      exit(-1);
    }
    return ptr;
  }

  // Set a page to be read-only but shared
  void * mmapRdShared(int pageNo) {
    void * start = (void *)((intptr_t)base() + pageNo * xdefines::PageSize);

    // Map to writable share area. 
    return mmapRdShared(start); 
  }

  // Set a page to be read-only but shared
  void * mmapRdShared(void * start) {
    void * ptr;

    // Map to writable share area. 
    ptr = changeMappingToShared(PROT_READ, start, xdefines::PageSize); 
    if(ptr == MAP_FAILED) {
      fprintf(stderr, "Weird, %d remove protect failed!!!\n", getpid());
      exit(-1);
    }
    return (ptr);
  }

  /// Set a block of memory to Readable/Writable and shared. 
  void *mmapRwShared(void * start, size_t size) {
    void * ptr;

    // Map to writable share area. 
    ptr = changeMappingToShared(PROT_READ|PROT_WRITE, start, size); 
    if(ptr == MAP_FAILED) {
      fprintf(stderr, "Weird, %d remove protect failed!!!\n", getpid());
      exit(-1);
    }
    return (ptr);
  }

  void openProtection (void) {
    mmapRdPrivate(base(), size());
	//  fprintf(stderr, "%d : protected %p with size %x\n", getpid(), base(), size());
  }

  // Can we change a block of memory to the shared mapping??
  // FIXME: if all memory has been committed, then it is safe to do this.
  void closeProtection(void) {
    mmapRwShared(base(), size());
  }

 
  /// @return true iff the address is in this space.
  inline bool inRange (void * addr) {
    if (((size_t) addr >= (size_t)_startaddr) && 
        ((size_t) addr < (size_t)_endaddr)) {
      return true;
    } else {
      return false;
    }
  }

  /// @return the start of the memory region being managed.
  inline char * base (void) const {
    return _transientMemory;
  }

  /// @return the size in bytes of the underlying object.
  inline size_t size (void) const {
    return _startsize;
  }

  /// @brief Record a write to this location.
  void handleWrite (void * addr) {
    // Compute the page number of this item
    int pageNo = computePage ((size_t) addr - (size_t) base());
	  unsigned long * pageStart = (unsigned long*)((intptr_t)_transientMemory + xdefines::PageSize * pageNo);
	  int origUsers = 0;

    assert(inRange(addr) == true);
 
	  // Get an entry from page store.
    struct pageinfo * curr = xpageentry::getInstance().alloc();
	  curr->pageNo = pageNo;
	  curr->pageStart = pageStart;

	  // Get current page's version number. 
	  // Trick here: we have to get version number before the force of copy-on-write.
	  // Getting the old version number is safer than getting of a new version number.
    // We use the version number to decide whether there is a need 
    // to do word-by-word commit. 
    // Getting old version can means unnecessary to do the a word-by-word commit, but it is 
    // safe to do this.
    
#if 0 
	  // Force the copy-on-write of kernel by writing to this address directly
 #if defined(X86_32BIT)
    asm volatile ("movl %0, %1 \n\t"
                  :   // Output, no output 
                  : "r"(pageStart[0]),  // Input 
                    "m"(pageStart[0])
                  : "memory");
 #else
    asm volatile ("movq %0, %1 \n\t"
                  :   // Output, no output 
                  : "r"(pageStart[0]),  // Input 
                    "m"(pageStart[0])
                  : "memory");
  #endif 

    // Create the "twinPage" from _transientMemory.
	  memcpy(curr->twinPage, pageStart, xdefines::PageSize);
#endif
	  _dirtiedPagesList.insert (dirtyPagePair(pageNo,curr));
  }

  /// @brief Start a transaction.
  inline void begin (void) {
	  // Update all pages related in this dirty page list
    updateAll();
  }

  void stats (void) {
    fprintf (stderr, "xmapping stats: %d dirtied\n", _dirtiedPagesList.size());
  }

  inline void writePageDiffs (const void * local, const void * twin, void * dest) {
    __m128i * localbuf = (__m128i *) local;
    __m128i * twinbuf  = (__m128i *) twin;
    __m128i * destbuf  = (__m128i *) dest;
    // Some vectorizing pragamata here; not sure if gcc implements them.
  #pragma vector always
    for (int i = 0; i < xdefines::PageSize / sizeof(__m128i); i++) {
      __m128i localChunk, twinChunk, destChunk;
  
      localChunk = _mm_load_si128 (&localbuf[i]);
      twinChunk  = _mm_load_si128 (&twinbuf[i]);
  
      // Compare the local and twin byte-wise.
      __m128i eqChunk = _mm_cmpeq_epi8 (localChunk, twinChunk);
  
      // Invert the bits by XORing them with ones.
      __m128i neqChunk = _mm_xor_si128 (allones, eqChunk);
  
      // Write local pieces into destbuf everywhere diffs.
      _mm_maskmoveu_si128 (localChunk, neqChunk, (char *) &destbuf[i]);
    }
  }

  /// Check the heap overflow. 
  inline bool sanitycheckPerform() {
    // Currently, we only need to check the heap overflow 
    assert(_isHeap == true);

  	struct pageinfo * pageinfo = NULL;
  	int pageNo;
  	unsigned long * persistent;
    bool hasOverflow = false;

    unsigned int lastpage = 0xFFFFFF00;
    void * batchedStart = NULL;
    int    batchedPages = 0;

    // We only check if there are some dirty pages.  
  	if(_dirtiedPagesList.size() != 0) {
      // Commit those private pages by change the page's version number and decrement the user counter.
      for (dirtyListType::iterator i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
  	    pageNo = i->first;

        if(pageNo == lastpage + 1) {
          batchedPages++; 
        }
        else {
          if(batchedPages != 0) {
            // How to check the overflow error.
            hasOverflow = sanitycheck::getInstance().checkHeapIntegrity
                              (batchedStart, batchedPages * xdefines::PageSize);

            if(hasOverflow) {
              break;
            }
          }
          batchedPages = 1;
          lastpage = pageNo;
          batchedStart = i->second->pageStart;
        }
  	 //   persistent = (unsigned long *) ((intptr_t)_persistentMemory + xdefines::PageSize * pageNo);
      }
    }

    // Now we are doing the last one checking.
    if(hasOverflow == false && batchedPages != 0) {
      hasOverflow = sanitycheck::getInstance().checkHeapIntegrity
                              (batchedStart, batchedPages * xdefines::PageSize);
    }

    return hasOverflow;
  }
 
  // Commit all private pages to the shared mapping 
  inline void commit(void) {
  	struct pageinfo * pageinfo = NULL;
  	int pageNo;
  	unsigned long * persistent;
  
  	if(_dirtiedPagesList.size() == 0) {
      return;
	  }

    // Commit those private pages by change the page's version number and decrement the user counter.
    for (dirtyListType::iterator i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
  	  pageinfo = i->second;
  	  pageNo = pageinfo->pageNo;
  
  	  persistent = (unsigned long *) ((intptr_t)_persistentMemory + xdefines::PageSize * pageNo);
 
      // For single thread program, we can use the memcpy to commit a page.
      memcpy(persistent, pageinfo->pageStart, xdefines::PageSize);
	  }
  }
  /// @brief Update every page frame from the backing file.
  /// Change to this function so that it will deallocate those backup pages. Previous way
  /// will have a memory leakage here without deallocation of Backup Pages.
  /// Also, re-protect those block in the list.
  void updateAll (void) {
    // Dump the now-unnecessary page frames, reducing space overhead.
    dirtyListType::iterator i;
    unsigned int lastpage = 0xFFFFFF00;
    void * batchedStart = NULL;
    int    batchedPages = 0;
    int pageNo;

    for (i = _dirtiedPagesList.begin(); i != _dirtiedPagesList.end(); ++i) {
      pageNo = i->first;

      if(pageNo == lastpage + 1) {
        batchedPages++; 
      }
      else {
        if(batchedPages != 0) {
          updatePages(batchedStart, batchedPages);
        }
        batchedPages = 1;
        lastpage = pageNo;
        batchedStart = i->second->pageStart;
      }
    }
   
    // Now we are doing the last one checking.
    if(batchedPages != 0) {
      updatePages(batchedStart, batchedPages);
    }
 
	  _dirtiedPagesList.clear();
    
	  // Clean up those page entries.
    xpageentry::getInstance().cleanup();
  }


  /// @brief Commit all writes.
  inline void memoryBarrier (void) {
    atomic::memoryBarrier();
  }

private:

  inline int computePage (int index) {
    return (index / xdefines::PageSize);
  }

  /// @brief Update the given page frame from the backing file.
  void updatePages (void * local, int pages) {
    size_t size = xdefines::PageSize * pages;

    madvise (local, size, MADV_DONTNEED);

	  // Set this page to PROT_READ again.
	  mprotect (local, size, PROT_READ);
  }
 
  /// True if current xmapping.h is a heap.
  bool _isHeap;

  /// The starting address of the region.
  void * _startaddr;

  /// The size of the region.
  size_t _startsize;

  /// The starting address of the region.
  void * _endaddr;

  /// A map of dirtied pages.
  dirtyListType _dirtiedPagesList;

  /// The file descriptor for the backing store.
  int _backingFd;

  /// The transient (not yet backed) memory.
  char * _transientMemory;

  /// The persistent (backed to disk) memory.
  char * _persistentMemory;

#ifdef SSE_SUPPORT
  // A string of one bits, only useful when we are having SSE_SUPPORT.
  __m128i allones;
#endif

};

#endif
