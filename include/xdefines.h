// -*- C++ -*-

/*
 
  Copyright (c) 2007-2012 Emery Berger, University of Massachusetts Amherst.

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

#ifndef SHERIFF_XDEFINES_H
#define SHERIFF_XDEFINES_H

#include <sys/types.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "log.h"
#include "real.h"
#include "list.h"
//#include "internalsyncs.h"


/*
 * @file   xdefines.h   
 * @brief  Global definitions for Sheriff-Detect and Sheriff-Protect.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 

#if 0
//#define DEBUG(...) 
typedef struct runtime_data {
  volatile unsigned long thread_index;
  volatile unsigned long threads;
  struct runtime_stats stats;
} runtime_data_t;

extern runtime_data_t *global_data;
#endif
#define GCC_VERSION (__GNUC__ * 10000 \
    + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if GCC_VERSION >= 40300
#include <tr1/unordered_map>
#define hash_map std::tr1::unordered_map
#else
#include <ext/hash_map>
#define hash_map __gnu_cxx::hash_map
#endif

extern size_t __max_stack_size; 
typedef void * threadFunction (void *);
extern int getThreadIndex();
extern char * getThreadBuffer();
extern void jumpToFunction(ucontext_t * cxt, unsigned long funcaddr);
extern bool addThreadQuarantineList(void * ptr, size_t size);

typedef enum e_faultyObjectType {
  OBJECT_TYPE_OVERFLOW = 1,
  OBJECT_TYPE_USEAFTERFREE,
  OBJECT_TYPE_LEAK,
  OBJECT_TYPE_INVALID,
} faultyObjectType;

inline size_t alignup(size_t size, size_t alignto) {
  return ((size + (alignto - 1)) & ~(alignto -1));
}

inline size_t aligndown(size_t addr, size_t alignto) {
  return (addr & ~(alignto -1));
}

struct syncEvent {
  list_t     list;
  // Which thread is performing synchronization? 
  void    *  thread;
  void    *  eventlist;
  int        ret; // used for mutex_lock
};

struct freeObject {
  void * ptr;
  union {
    int owner; // which thread is using this heap.
    size_t size;
  };
};

class xdefines {
public:
  enum { USER_HEAP_SIZE     = 1048576UL * 8192 }; // 8G
//  enum { USER_HEAP_SIZE     = 1048576UL * 1024 }; // 8G
  enum { USER_HEAP_BASE     = 0x100000000 }; // 4G
  enum { MAX_USER_SPACE     = USER_HEAP_BASE + USER_HEAP_SIZE };
  enum { INTERNAL_HEAP_BASE = 0x100000000000 };
  //enum { INTERNAL_HEAP_BASE = 0xC0000000 };
  enum { INTERNAL_HEAP_SIZE = 1048576UL * 128 };

  enum { QUARANTINE_BUF_SIZE = 1024 };
  
  // If total free objects is larger than this size, we begin to 
  // re-use those objects
  enum { QUARANTINE_TOTAL_SIZE = 1048576 * 16 };
    
  // 128M so that almost all memory is allocated from the begining. 
  enum { USER_HEAP_CHUNK = 1048576 * 4 }; 
  enum { INTERNAL_HEAP_CHUNK = 1048576 };
  enum { OBJECT_SIZE_BASE = 16 };
  //enum { MAX_OBJECTS_USER_HEAP_CHUNK = USER_HEAP_CHUNK/MINIMUM_OBJECT_SIZE };
  //enum { TOTAL_USER_HEAP_HUNKS = USER_HEAP_SIZE/USER_HEAP_CHUNK };


  // 4 bytes is representing by 1 bit. If bit is 0, it is not a canary word.
  // Otherwise, it is a canary word. 
  enum { BIT_SECTOR_SIZE = 32 }; 

  enum { MAX_WATCHPOINTS  = 4 };
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize-1) };
  enum { MAX_ALIVE_THREADS = 4 }; 
  enum { NUM_HEAPS = MAX_ALIVE_THREADS };
  enum { SYNCMAP_SIZE = 4096 }; 
  enum { THREAD_MAP_SIZE = 1024 }; 
  enum { MAX_STACK_SIZE = 0xa00000UL}; // 64pages
  enum { TEMP_STACK_SIZE = 0xa00000UL}; // 64 pages
  enum { NUM_GLOBALS = 10 }; // At least, we need app globals, libc globals and libthread globals.
  //enum { MAX_GLOBALS_SIZE = 1048576UL * 10 };
  enum { CACHE_LINE_SIZE = 64};

  /**
   * Definition of sentinel information.
   */
  enum { WORD_SIZE = sizeof(size_t) };
  enum { WORD_SIZE_MASK = WORD_SIZE - 1 };
  enum { SENTINEL_SIZE = WORD_SIZE };
  enum { MAGIC_BYTE_NOT_ALIGNED = 0xEE };
  enum { FREE_OBJECT_CANARY_WORDS = 16 };
  enum { FREE_OBJECT_CANARY_SIZE = 16 * WORD_SIZE };
  enum { CALLSITE_MAXIMUM_LENGTH = 10 }; 

  // FIXME: the following definitions are sensitive to 
  // glibc version (possibly?)
  enum { FILES_MAP_SIZE = 4096 }; 
  enum { MEMTRACK_MAP_SIZE = 4096 }; 
  enum { FOPEN_ALLOC_SIZE = 0x238 };
  enum { DIRS_MAP_SIZE = 1024 }; 
  enum { DIROPEN_ALLOC_SIZE = 0x8038 };

//  enum { MAX_RECORD_ENTRIES = 0x1000 };
  enum { MAX_RECORD_ENTRIES = 0x1000000 };
  enum { MAX_FREE_OBJECTS = 0x100000 };
  enum { MAX_SYNCEVENT_ENTRIES = 0x1000000 };

#ifdef X86_32BIT
  enum {SENTINEL_WORD = 0xCAFEBABE };
  enum {MEMALIGN_SENTINEL_WORD = 0xDADEBABE };
#else
  enum {SENTINEL_WORD = 0xCAFEBABECAFEBABE };
  enum {MEMALIGN_SENTINEL_WORD = 0xDADEBABEDADEBABE };
#endif
};


#endif
