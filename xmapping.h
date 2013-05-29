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

#include <algorithm> //sort
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>

#include "atomic.h"

#include "xdefines.h"
#include "ansiwrapper.h"

#include "mm.h"
#include "sanitycheck.h"

using namespace std;

class xmapping {
public:

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
    // Check whether the size is valid, should be page aligned.
    if(size % xdefines::PageSize != 0) {
      PRFATAL("Wrong size %lx, should be page aligned.\n", size);
    }

    // Establish two maps to the backing file.
    // The persistent map is shared.
    _backupMemory = (char *) MM::mmapAllocatePrivate(size);

    // If we specified a start address (globals), copy the contents into the
    // persistent area now because the transient memory mmap call is going
    // to squash it.
    if(startaddr) {
	    _isHeap = false;
      _userMemory = (char *)startaddr;
    }
  	else {
	    _isHeap = true;
      _userMemory = (char *) MM::mmapAllocatePrivate(size);
	  }

    // The transient map is private and optionally fixed at the
    // desired start address.
    _startsize = size;
    _startaddr = (void *)_userMemory;
    _endaddr = (void *)((intptr_t)_userMemory + _startsize);

    // bitmap bits
#ifndef NDEBUG
   //fprintf (stderr, "transient = %p, persistent = %p, size = %lx, _startaddr %p, _endaddr %p\n", _userMemory, _backupMemory, _startsize, _startaddr, _endaddr);
#endif

#ifdef SSE_SUPPORT	
    // A string of one bits.
    allones = _mm_setzero_si128();
    allones = _mm_cmpeq_epi32(allones, allones); 
#endif
  }

  // Do nothing 
  void finalize(void) {
  }

  /// @return the start of the memory region being managed.
  inline char * base (void) const {
    return _userMemory;
  }

  /// @return the size in bytes of the underlying object.
  inline size_t size (void) const {
    return _startsize;
  }

#ifdef DETECT_OVERFLOW
  inline bool checkHeapOverflow(void) {
    assert(_isHeap == true);
 
    bool hasOverflow = false;

    hasOverflow = sanitycheck::getInstance().checkHeapIntegrity
                  (base(), size());

    return hasOverflow;
  }
#endif

  // For the page
  void backup(void * end) {
    size_t sz;

    if(_isHeap) {
      sz = (intptr_t)end - (intptr_t)base();
    }
    else {
      // Commit all pages
      sz = size(); 
    }

    // Copy everything to _backupMemory From _userMemory
    memcpy(_backupMemory, _userMemory, sz);
  }

  // How to commit some memory
  void commit(void * start, size_t size) {
    size_t offset = (intptr_t)start - (intptr_t)base();
 
    void * dest = (void *)((intptr_t)_backupMemory + offset);
    memcpy(dest, start, size);
  }

  // Release all temporary pages.
  void recoverMemory(void *end) {
    size_t sz;

    if(_isHeap) {
      sz = (intptr_t)end - (intptr_t)base();
    }
    else {
      // Commit all pages
      sz = size();
    }

//    PRWRN("ccccccccccopying back memory to userMemory %p, sz %lx\n", _userMemory, sz);  
    memcpy(_userMemory, _backupMemory, sz);
  }


private:

  /// True if current xmapping.h is a heap.
  bool _isHeap;

  /// The starting address of the region.
  void * _startaddr;

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
