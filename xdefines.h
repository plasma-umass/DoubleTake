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
#include "libfuncs.h"
#include "prof.h"

/*
 * @file   xdefines.h   
 * @brief  Global definitions for Sheriff-Detect and Sheriff-Protect.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 

#if 0
//#define fprintf(stderr, ...) 
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

extern "C" {
  extern size_t __max_stack_size; 
  typedef void * threadFunction (void *);
  extern int getThreadIndex();
  extern char * getThreadBuffer();
  extern void jumpToFunction(ucontext_t * cxt, unsigned long funcaddr);
  #define EXIT (WRAP(exit)(-1))
};

class xdefines {
public:
#ifdef X86_32BIT
  enum { PROTECTEDHEAP_SIZE = 1048576UL * 512 };
#else
  enum { PROTECTEDHEAP_SIZE = 1048576UL * 8192 };
  //enum { PROTECTEDHEAP_SIZE = 1048576UL * 4096 };
#endif
//  enum { SHAREDHEAP_SIZE = 1048576UL * 2048 };
  //enum { PROTECTEDHEAP_CHUNK = 40960 };
  enum { PHEAP_CHUNK = 40960 };
  enum { SHAREDHEAP_CHUNK = 1048576};
  enum { LARGE_CHUNK = 1024 };
  //enum { INTERNAL_HEAP_SIZE = 1048576UL * 2048 };
  enum { INTERNAL_HEAP_SIZE = 1048576UL * 2048 };

  // Reserve how many bitmap segment for each process
  // Now consider that we wil have 512M data, for each page,
  // We will need a bitmap segment. Totally, we will need around
  // 128 K bitmap. 
  enum { PRIVATE_PAGES = 20000 }; 

  // 4 bytes is representing by 1 bit. If bit is 0, it is not a canary word.
  // Otherwise, it is a canary word. 
  enum { BIT_SECTOR_SIZE = 32 }; 

  enum { MAX_WATCHPOINTS  = 4 };
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize-1) };
  //enum { NUM_HEAPS = 32 };
#ifdef SINGLE_THREAD
  // We can only supported 128 threads simultaneously
  enum { MAX_ALIVE_THREADS = 1 }; 
#else 
  enum { MAX_ALIVE_THREADS = 64 }; 
  //enum { MAX_ALIVE_THREADS = 128 }; 
#endif
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

  // FIXME: the following definitions are sensitive to 
  // glibc version (possibly?)
  enum { FILES_MAP_SIZE = 4096 }; 
  enum { FOPEN_ALLOC_SIZE = 0x238 };
  enum { DIRS_MAP_SIZE = 1024 }; 
  enum { DIROPEN_ALLOC_SIZE = 0x8038 };

#ifdef X86_32BIT
  enum {SENTINEL_WORD = 0xCAFEBABE };
  enum {MEMALIGN_SENTINEL_WORD = 0xDADEBABE };
#else
  enum {SENTINEL_WORD = 0xCAFEBABECAFEBABE };
  enum {MEMALIGN_SENTINEL_WORD = 0xDADEBABEDADEBABE };
#endif
};


#endif
