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
#include <list>
#include <ucontext.h>
#include <internalheap.h>
#include "threadmap.h"
#include "objectheader.h"
#include "spinlock.h"
//#include "leakcheck.h"

#include "memtrack.h"

class leakcheck {

  leakcheck() { }

public:

  static leakcheck& getInstance() {
    static char buf[sizeof(leakcheck)];
    static leakcheck * theOneTrueObject = new (buf) leakcheck();
    return *theOneTrueObject;
  }
  
  void searchHeapPointersInsideGlobals();
  
  bool doSlowLeakCheck(void * begin, void *end) {
    _heapBegin = (unsigned long)begin + sizeof(objectHeader);
    _heapEnd = (unsigned long)end;
    _nonStartAddrs = 0;
    _totalLeakageSize = 0;

 //   PRINF("doSlowLeakCheck begin %p end %p\n", begin, end);
    // Search all existing registers to find possible heap pointers
    ucontext_t context;

    getcontext(&context);
    //PRINF("doSlowLeakCheck line %d\n", __LINE__);

    searchHeapPointers(&context);
    //PRINF("doSlowLeakCheck line %d\n", __LINE__);
    
    // Search all stacks to find possible heap pointers
    searchHeapPointersInsideStack(&context);
    //PRINF("doSlowLeakCheck line %d\n", __LINE__);
 
    // Search the globals to find possible heap pointers
    searchHeapPointersInsideGlobals();    
    //PRINF("doSlowLeakCheck line %d\n", __LINE__);

    // Traverse all possible heap pointers inside unexplored sets.
    traverseUnexploredList();
    return reportUnreachableNonfreedObjects();
  } 


  // In the end of program, we can only check those non-freed objects.
  // All of them are considered as memory leakage
  bool doFastLeakCheck(void * begin, void * end) {
    _heapBegin = (unsigned long)begin;
    _heapEnd = (unsigned long)end;
    _totalLeakageSize = 0;

    return doSlowLeakCheck(begin, end);
 //  return reportUnreachableNonfreedObjects();
  }


  // In the end of program, all objects 
  // inside 
private:
  static objectHeader * getObject (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }


  // Check a heap object covering given addr
  void exploreHeapObject(unsigned long addr) {
    // In most cases, this addr is the starting address of a heap object.
    objectHeader * object = getObject((void *)addr);
    unsigned long objectStart = 0;
      
//    PRINF("exploreHeapObject line %d on heap address %lx\n", __LINE__, addr);

    if(!object->isGoodObject()) {
      // Current address is not the starting address of a heap object.
      // To get the starting addresses of this object, we may rely on the 
      // help of sentinel map.
      // Maybe we should check whether this object is an memaligned object? 
      //PRINF("exploreHeapObject line %d\n", __LINE__);
      if(sentinelmap::getInstance().findObjectStartAddr((void *)addr, &objectStart)) { 
      //  PRINF("exploreHeapObject line %d objectStart %lx\n", __LINE__, objectStart);
        object = getObject((void *)objectStart);
       
        // If the return address is not the starting address of
        // an heap object, then this address is possibly wrong. 
        if(!object->isGoodObject() || !object->isValidAddr(addr)) {
          object = NULL;
        }
      }
      else {
        //PRINF("findObjectStartAddr failed on addr %lx at line %d\n", addr, __LINE__);
        object = NULL;
      }
    }
    else {
//      PRINF("exploreHeapObject line %d Good object!!!!!!!\n", __LINE__);
      if(object->isValidAddr(addr)) {
//      PRINF("exploreHeapObject line %d Good object!!!!!!!\n", __LINE__);
        objectStart = addr;
      }
      else {
        object = NULL;
      }
    }
      
    //PRINF("exploreHeapObject line %d\n", __LINE__);
      
    // If current object is free or this object has been checked, we don't care
    if(!object) {
      return;
    }
      
    assert(object->isGoodObject());
    assert(object->isValidAddr(addr));

    // Get the end of object.
    if(object->doCheckObject()) {
      unsigned long end = objectStart + object->getObjectSize();
      searchHeapPointers(objectStart, end);
//      PRINF("exploreHeapObject line %d addr %lx end %lx******\n", __LINE__, addr, end);

      // Mark that this object are reachable from roots.
      object->markObjectChecked();
    }
  }

  void traverseUnexploredList() {
    objectListType  i;
    unsigned long addr;

//    PRINF("traverseUnexploredList now empty %d\n", _unexploredObjects.empty());
    while(_unexploredObjects.empty() != true) {
      addr = _unexploredObjects.front();
      _unexploredObjects.pop_front();
    //  PRINF("EEEEEEEEEEExplored list with addr %lx\n", addr); 
      exploreHeapObject(addr);
    }
  }

  void insertLeakageMap(void * ptr, size_t objectSize, size_t size) {
    _totalLeakageSize += size;
    // Update the total size.
    PRWRN("**********************DoubleTake: Leakage at ptr %p with size %ld. objectsize %ld. Total leakage size %ld**************\n", ptr, size, objectSize, _totalLeakageSize);
    // We only start to rollback when leakage is too large?
    memtrack::getInstance().insert(ptr, objectSize, OBJECT_TYPE_LEAK);
  }

  // In the end, we should report all of those non-reachable non-freed objects.
  bool reportUnreachableNonfreedObjects() {
    unsigned long * ptr, *stop;
    bool hasLeakage = true;

    // We basically report those non-checked and non-freed objects in
    // the system by traverse all objects.
    ptr = (unsigned long *)_heapBegin;
    stop = (unsigned long *)_heapEnd;
 
//    PRINF("************CHECK LEAK at line %ld****************\n", __LINE__); 
    // For the last object, we may use the bitmap to help us.
    // For example, we may only allocate some of a chunk,
    // Then how can we know whether we should move forward or not. 
    unsigned long offset; 
    objectHeader * object;
    while(ptr < stop) {
      // We find a object header
      if(*ptr == xdefines::SENTINEL_WORD && sentinelmap::getInstance().isSet(ptr)) {
 //       PRINF("Find object at %p\n", ptr);
        object = getObject((void *)++ptr);
//        if(object->isObjectFree()) {
//          PRINF("object at %p is free\n", ptr);
//        }

        if(object->checkLeakageAndClean()) {
 //         PRINF("Leakage at %p\n", object->getStartPtr());
          hasLeakage = true;
          // Adding this object to the global leakage map, which should be tracked in re-execution
          insertLeakageMap(object->getStartPtr(), object->getObjectSize(), object->getSize());
        }
        ptr = (unsigned long *)object->getNextObject();
      }
      else {
        ptr++;
      }
    }
    
//    PRINF("Totally leakage memory is around %lx bytes\n", _totalLeakageSize);  

    return hasLeakage;
  }

  bool isPossibleHeapPointer(unsigned long addr) {
    return (addr > _heapBegin && addr < _heapEnd) ? true : false; 
  }

  // Insert an address into unexplored set.
  void checkInsertUnexploredList(unsigned long addr) {
    if(isPossibleHeapPointer(addr)) {
  //    PRINF("IIIIIIIIIIIIIunexplored list with addr %lx\n", addr); 
      //lock();
      _unexploredObjects.push_back(addr);
      //unlock();
    }
  }

  // Seatch heap pointers inside a memory region
  void searchHeapPointers(unsigned long start, unsigned long end) {
    assert(((intptr_t)start)%sizeof(unsigned long) == 0);

    // It is good if the end is not aligned caused by non-aligned malloc. 
    // However, we only care about those aligned
    // address since they can only possibly hold heap addresses.
    unsigned long * stop = (unsigned long *)aligndown(end, sizeof(void *));
    unsigned long * ptr = (unsigned long *)start;
    //PRINF("searchHeapPointers at ptr %p stop %p\n", ptr, stop);
    while(ptr < stop) {
      checkInsertUnexploredList(*ptr);
      ptr++;
    }
  }

  // Search heap pointers inside registers set.
  void searchHeapPointers(ucontext_t * context) {
    for(int i = REG_R8; i <= REG_RCX; i++) {
      checkInsertUnexploredList(context->uc_mcontext.gregs[i]);
    }
  }
  
  // Seearch heap pointers inside all stack area
  void searchHeapPointersInsideStacks() {
#if 0
    threadmap::aliveThreadIterator i;
    for(i = threadmap::getInstance().begin();
      i != threadmap::getInstance().end(); i++)
    {
      thread_t * thread = i.getThread();
    }
#endif
  }
  
  void searchHeapPointersInsideStack(void * start) {
    void * stop = current->stackTop;

    //PRINF("search heap pointers inside stack now. start %p stop %p\n", start, stop);
    searchHeapPointers((intptr_t)start, (intptr_t)stop);
  }

  void lock() {
    _lck.lock();
  }

  void unlock() {
    _lck.unlock();
  }


  typedef std::list<unsigned long, HL::STLAllocator<unsigned long, InternalHeapAllocator> > objectListType;
  objectListType _unexploredObjects;

  
  size_t _totalLeakageSize;

  spinlock _lck;

#define MAXIMUM_SIZE_POWER 30
  // From 2^4 to 2 ^ 30
  unsigned long _sizeArray[30];
  objectListType _sizeList[30];

  // It is used to count how many non-start addresses in
  // the calculation of reachability
  size_t _nonStartAddrs;
 
//  typedef std::set<struct memoryRegion *, less<void *
  unsigned long _heapBegin;
  unsigned long _heapEnd;
};
#endif
