// -*- C++ -*-

/*
  Copyright (c) 2012, University of Massachusetts Amherst.

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
 * @file   selfmap.h
 * @brief  Analyze the self mapping. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <sys/resource.h>
#include <iostream>
#include <fstream>
#include <execinfo.h>

//#include <libunwind-ptrace.h>
//#include <sys/ptrace.h>

#define MAX_BUF_SIZE 4096
#include "xdefines.h"
#include "threadstruct.h"
#include "selfmap.h"

// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
void selfmap::printCallStack(ucontext_t * context, void * addr, bool isOverflow) {
  void * array[10];
  int size;
  int skip = 0;

  PRINF("Try to get backtrace with array %p\n", array);
  // get void*'s for all entries on the stack
  current->internalheap = true;
  size = backtrace(array, 10);
  current->internalheap = false;
  PRINF("After get backtrace with array %p\n", array);

  for(int i = 0; i < size; i++) {
    if(isDoubleTakeLibrary(array[i])) {
      skip++;
    } 
    else {
      break;
    }
  }

  backtrace_symbols_fd(&array[skip], size-skip, 2);

  // Print out the source code information if it is a overflow site.
  if(isOverflow) {
    PRINF("\nSource code information about overflow site:\n");
    char buf[MAX_BUF_SIZE];

    for(int i = skip; i < size-skip; i++) {
      if(isApplication(array[i])) {
        PRINF("callstack frame %d: ", i);
        // Print out the corresponding source code information
        sprintf(buf, "addr2line -e %s %p", _filename,  (void*)((uintptr_t)array[i]-2));
        system(buf);
      }
    }
  }
}
 
void selfmap::printCallStack(int depth, void ** array) {
  char buf[MAX_BUF_SIZE];
  int index = 0;
  //fprintf(stderr, "printCallStack:_filename %s\n", _filename);
  for(int i = 0; i < depth; i++) {
    if(isApplication(array[i])) {
      index++;
      fprintf(stderr, "callstack frame %d: %p\n", index, array[i]);
      // Print out the corresponding source code information
      sprintf(buf, "addr2line -e %s 0x%lx", _filename, (unsigned long)array[i]);
      system(buf);
    }
  }
}
// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
int selfmap::getCallStack(void ** array) {
  int size;

  PRINF("Try to get backtrace with array %p\n", array);
  // get void*'s for all entries on the stack
  current->internalheap = true;
  size = backtrace(array, xdefines::CALLSITE_MAXIMUM_LENGTH);
  current->internalheap = false;
  PRINF("After get backtrace with array %p\n", array);

  return size;
}
 
