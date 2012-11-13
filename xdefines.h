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

/*
 * @file   xdefines.h   
 * @brief  Global definitions for Sheriff-Detect and Sheriff-Protect.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 


#ifdef __cplusplus

extern "C"
{

  extern int textStart, textEnd;
  extern unsigned long * global_thread_index;
};

class xdefines {
public:
  enum { MAX_STACK_SIZE = 131072UL}; // 32pages, actually the normal size of stack is 21 pages.
  enum { TEMP_STACK_SIZE = 4096UL * 10}; // 10 pages, used when we do recovery.
#ifdef X86_32BIT
  enum { PROTECTEDHEAP_SIZE = 1048576UL * 600 };
#else
  enum { PROTECTEDHEAP_SIZE = 1048576UL * 1024 };
#endif
  enum { SHAREDHEAP_SIZE = 1048576UL * 100 };
  //enum { PROTECTEDHEAP_CHUNK = 40960 };
  enum { PHEAP_CHUNK = 40960 };
  enum { SHAREDHEAP_CHUNK = 1048576};
  enum { LARGE_CHUNK = 1024 };
  enum { INTERNALHEAP_SIZE = 1048576UL * 20 };

  // Reserve how many private pages for each process
  enum { PRIVATE_PAGES = 2000 }; 

  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize-1) };
  //enum { NUM_HEAPS = 32 };
  enum { NUM_HEAPS = 2 };
  enum { NUM_GLOBALS = 3 }; // At least, we need app globals, libc globals and libthread globals.
  //enum { MAX_GLOBALS_SIZE = 1048576UL * 10 };
  enum { CACHE_LINE_SIZE = 64};
};

#endif

#endif
