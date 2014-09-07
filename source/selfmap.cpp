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
#include "xthread.h"
#include "selfmap.h"

// Normally, callstack only saves next instruction address. 
// To get current callstack, we should substract 1 here. 
// Then addr2line can figure out which instruction correctly
#define PREV_INSTRUCTION_OFFSET 1

// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
void selfmap::printCallStack() {
  void * array[10];
  int size;

  // get void*'s for all entries on the stack
	xthread::disableCheck();
  size = backtrace(array, 10);
	xthread::enableCheck();
  //backtrace_symbols_fd(&array[0], size, 2);

  // Print out the source code information if it is a overflow site.
  selfmap::getInstance().printCallStack(size, &array[0]);
}

// Calling system involves a lot of irrevocable system calls.
void selfmap::printCallStack(int depth, void ** array) {
  char buf[MAX_BUF_SIZE];
  int index = 0;
  //fprintf(stderr, "printCallStack:_filename %s\n", _filename);
  xthread::disableCheck();
  for(int i = 0; i < depth; i++) {
    void * addr = (void *)((unsigned long)array[i] - PREV_INSTRUCTION_OFFSET);
    if(isApplication(addr)) {
    	index++;
      PRINT("\tcallstack frame %d: %p\t", index, addr);
      // Print out the corresponding source code information
      sprintf(buf, "addr2line -e %s %p", _filename, addr);
    //  PRINT("%s\n", buf);
      system(buf);
    }
#if 0
		// We print out the first one who do not belong to library itself
		//else if(index == 1 && !isDoubleTakeLibrary((void *)addr)) {
		else if(!isDoubleTakeLibrary((void *)addr)) {
    	index++;
      PRINT("\tcallstack frame %d: %p\n", index, addr);
		}
#endif
  }
  xthread::enableCheck();
}
// Print out the code information about an eipaddress
// Also try to print out stack trace of given pcaddr.
int selfmap::getCallStack(void ** array) {
  int size;

  PRINF("Try to get backtrace with array %p\n", array);
  // get void*'s for all entries on the stack
	xthread::disableCheck();
  size = backtrace(array, xdefines::CALLSITE_MAXIMUM_LENGTH);
	xthread::enableCheck();
  PRINF("After get backtrace with array %p\n", array);

  return size;
}
 
