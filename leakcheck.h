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
 * @file   leakcheck.h
 * @brief  Detecting leakage usage case.
           Basic idea:
           We first traverse the heap to get an alive list (not freed objects) and verify whether
           these objects are reachable from stack, registers and global variables or not.
           If an object is not freed and it is not reachable, then it is a memory leak. 

           In order to verify whether an object is reachable, we start from the root list (those 
           possible reachable objects).

           However, it is much easier for the checking in the end of a program. We basically think
           that every object should be freed. Thus, we only needs to search the heap list to find
           those unfreed objects. If there is someone, then we reported that and rollback.  

           In order to detect those callsites for those memory leakage, we basically maintain 
           a hashtable. Whenever there is memory allocation, we check whether this object is a 
           possible memory leakage. If yes, then we update corresponding list about how many leakage
           happens on each memory allocation site. 

 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _LEAKAGE_DETECT_H_
#define _LEAKAGE_DETECT_H_

#include <set>
#include <internalheap.h>

class leakCheck {
  struct aliveObject {
    void * begin;
    void * end;
    // If it is checked, then it is not a leak object if we view this conservatively
    bool   checked; 
  };

public:
  bool doLeakCheck(void * begin, void *end, ) {
    _heapBegin = begin;
    _heapEnd = end;

    
  } 

  void getUnexploredSetFromStack(void) {


  }

  void getObjectsFromRegisters(void) {

  }


  // In the end of program, all objects 
  // inside 
private:
  typedef std::set<void *, less<void *>, HL::STLAllocator<void *, InternalHeapAllocator> > pagesetType;
  pagesetType _unexploredSet;

  
//  typedef std::set<struct memoryRegion *, less<void *
  unsigned long _heapBegin;
  unsigned long _heapEnd;
};
#endif
