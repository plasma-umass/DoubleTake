/* -*- C++ -*- */

/*

  Heap Layers: An Extensible Memory Allocation Infrastructure
  
  Copyright (C) 2000-2012 by Emery Berger
  http://www.cs.umass.edu/~emery
  emery@cs.umass.edu
  
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

/**
 * @class ZoneHeap
 * @brief A zone (or arena, or region) based allocator.
 * @author Emery Berger
 * @date June 2000
 *
 * Uses the superclass to obtain large chunks of memory that are only
 * returned when the heap itself is destroyed.
 *
*/

#ifndef HL_ZONEHEAP_H
#define HL_ZONEHEAP_H

#include <assert.h>

#include "utility/align.h"
#include "wrappers/mallocinfo.h"
#include "utility/sassert.h"

namespace HL {

  template <class SuperHeap, size_t ChunkSize>
  class ZoneHeap : public SuperHeap {
  public:

    enum { Alignment = SuperHeap::Alignment };

    ZoneHeap()
      : _sizeRemaining (-1),
	_currentArena (NULL),
	_pastArenas (NULL)
    {}

    ~ZoneHeap()
    {
      // printf ("deleting arenas!\n");
      // Delete all of our arenas.
      Arena * ptr = _pastArenas;
      while (ptr != NULL) {
	void * oldPtr = (void *) ptr;
	ptr = ptr->nextArena;
	//printf ("deleting %x\n", ptr);
	SuperHeap::free (oldPtr);
      }
      if (_currentArena != NULL)
	//printf ("deleting %x\n", _currentArena);
	SuperHeap::free ((void *) _currentArena);
    }

    inline void * malloc (size_t sz) {
      void * ptr = zoneMalloc (sz);
      assert ((size_t) ptr % Alignment == 0);
      return ptr;
    }

    /// Free in a zone allocator is a no-op.
    inline void free (void *) {}

    /// Remove in a zone allocator is a no-op.
    inline int remove (void *) { return 0; }


  private:

    ZoneHeap (const ZoneHeap&);
    ZoneHeap& operator=(const ZoneHeap&);

    inline void * zoneMalloc (size_t sz) {
      void * ptr;

			//fprintf(stderr, "zoneMalloc size sz %lx _currentArena %p _sizeRemaining %lx. int sz %x\n", sz, _currentArena, _sizeRemaining, (int)sz);
      // Round up size to an aligned value.
      sz = HL::align<HL::MallocInfo::Alignment>(sz);

			//fprintf(stderr, "after alingment sz %lx: zoneMalloc size sz %lx _currentArena %p _sizeRemaining %lx\n", sz, sz, _currentArena, _sizeRemaining);
	
			//fprintf(stderr, "first is %d second is %d\n", (_currentArena == NULL), (_sizeRemaining < (unsigned int) sz));	
			//fprintf(stderr, "first is %d second removing int is %d\n", (_currentArena == NULL), (_sizeRemaining <  sz));	
      // Get more space in our arena if there's not enough room in this one.
			// TONGPING: first bug, we should not use (int) to transfer
      if ((_currentArena == NULL) || (_sizeRemaining < sz)) {
			//fprintf(stderr, "zoneMalloc size sz %lx _currentArena %p _sizeRemaining %lx line %d\n", sz, _currentArena, _sizeRemaining, __LINE__);
			
		//	fprintf(stderr, "zoneMalloc size sz %lx _currentArena %p _sizeRemaining %lx _currentArena at %p\n", sz, _currentArena, _sizeRemaining, &_currentArena);
	// First, add this arena to our past arena list.
	if (_currentArena != NULL) {
	  _currentArena->nextArena = _pastArenas;
	  _pastArenas = _currentArena;
	}
	// Now get more memory.
	size_t allocSize = ChunkSize;
	if (allocSize < sz) {
	  allocSize = sz;
	}
	_currentArena =
	  (Arena *) SuperHeap::malloc (allocSize + sizeof(Arena));
	//fprintf(stderr, "_currentArena is %p\n", _currentArena);
	if (_currentArena == NULL) {
	  return NULL;
	}
	_currentArena->arenaSpace = (char *) (_currentArena + 1);
	_currentArena->nextArena = NULL;
	
		// TONGPING Second bug: 
		//_sizeRemaining = ChunkSize;
			_sizeRemaining = allocSize;
      }
      // Bump the pointer and update the amount of memory remaining.
      _sizeRemaining -= sz;
      ptr = _currentArena->arenaSpace;
      _currentArena->arenaSpace += sz;
      assert (ptr != NULL);
      assert ((size_t) ptr % SuperHeap::Alignment == 0);
      return ptr;
    }
  
    class Arena {
    public:
      Arena() {
	sassert<(sizeof(Arena) % HL::MallocInfo::Alignment == 0)> verifyAlignment;
      }

      Arena * nextArena;
      char * arenaSpace;
    };
    
    /// Space left in the current arena.
    long _sizeRemaining;

    /// The current arena.
    Arena * _currentArena;

    /// A linked list of past arenas.
    Arena * _pastArenas;
  };

}

#endif
