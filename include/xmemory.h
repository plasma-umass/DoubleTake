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
 * @file   xmemory.h
 * @brief  Memory management for all.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 

#ifndef _XMEMORY_H_
#define _XMEMORY_H_

#include <signal.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <set>

#include "xglobals.h"

#include "xpheap.h"

#include "xoneheap.h"
#include "xheap.h"

#include "objectheader.h"
#include "finetime.h"
#include "watchpoint.h"
#include "globalinfo.h"
#include "xthread.h"

// Encapsulates all memory spaces (globals & heap).
class xmemory {
private:

  // Private on purpose. See getInstance(), below.
  xmemory (void) 
  {
  }

public:
  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here, for use by the
  // signal handler.
  static xmemory& getInstance (void) {
    static char buf[sizeof(xmemory)];
    static xmemory * theOneTrueObject = new (buf) xmemory();
    return *theOneTrueObject;
  }

  void initialize(void) {
    // Install a handler to intercept SEGV signals (used for trapping initial reads and
    // writes to pages).
    installSignalHandler();

    // Call _pheap so that xheap.h can be initialized at first and then can work normally.
    _heapBegin = (intptr_t) _pheap.initialize((void *)xdefines::USER_HEAP_BASE, xdefines::USER_HEAP_SIZE);

    _heapEnd = _heapBegin + xdefines::USER_HEAP_SIZE;
	  _globals.initialize();
  }

  void finalize(void) {
	  _globals.finalize();
	  _pheap.finalize();
  }
 
  inline int getGlobalRegionsNumb(void) {
    return _globals.getRegions();
  }

  inline void getGlobalRegion(int index, unsigned long * begin, unsigned long * end) {
    _globals.getGlobalRegion(index, begin, end);
  }
 
  /* Heap-related functions. */
  inline void * malloc (size_t sz) {
    void * ptr;
    if(current->internalheap == true) {
      ptr = InternalHeap::getInstance().malloc(sz);
    }
    else {
      ptr = realmalloc(sz);
    }
    return ptr;
  }

  inline void * calloc (size_t nmemb, size_t sz) {
    void * ptr = malloc(nmemb * sz);
    return ptr;
  }

  inline void * realloc(void * ptr, size_t sz) {
    if (sz == 0) {
      free (ptr);
      return NULL;
    }

    // Do actual allocation
    void * newptr = malloc(sz);
    if (ptr == NULL) {
      return newptr;
    }

    size_t os = getSize (ptr);
    if (newptr && os != 0) {
      size_t copySz = (os < sz) ? os : sz;
      memcpy (newptr, ptr, copySz);
    }

    free(ptr);
    return newptr;
  }


  // Actual allocations
  inline void * realmalloc (size_t sz) {
  	unsigned char * ptr = NULL;
    int mysize = sz;
#if 0
    if(sz == 0) {
//      fprintf(stderr, "allocation size %d zzzzzzzzzzzzero\n", sz);
      return NULL;
    }
#endif

    if(sz < 16) {
      mysize = 16;
    }
    //fprintf(stderr, "In the beginning, THREAD%d at %p: malloc size %lx ptr %p\n", getThreadIndex(), pthread_self(), sz, ptr);
    ptr = (unsigned char *)_pheap.malloc(mysize);
    objectHeader * o = getObject (ptr);

    // Get the block size
    size_t size = o->getSize();
    
    // Set actual size there.
    o->setObjectSize(sz);
#ifdef DETECT_OVERFLOW
    assert(size >= sz);
    // Add another guard zone if block size is larger than actual size
    // in order to capture the 1 byte overflow.
    if(size > sz) {
      size_t offset = size - sz;

      // We are using the ptr to varify the size
      unsigned char * p = (unsigned char *)((intptr_t)ptr + sz);

      // If everything is aligned, add the guardzone.
      size_t nonAlignedBytes = sz & xdefines::WORD_SIZE_MASK;
      if(nonAlignedBytes == 0) {
        sentinelmap::getInstance().setSentinelAt(p);
      } 
      else {
        // For those less than one word access, maybe we do not care since memory block is 
        // always returned by 8bytes aligned or 16bytes aligned.
        // However, some weird test cases has this overflow. So we should detect this now.
        void * startp = (void *)((intptr_t)p - nonAlignedBytes);
        size_t setBytes = xdefines::WORD_SIZE - nonAlignedBytes; 
//#ifdef DETECT_NONALIGNED_OVERFLOW
        // Check how much magic bytes we should put there. 
        // The first byte of this is to specify how many bytes there.
        // For example, at 64bit, when nonAlignedBytes is 5,
        // Totally, we should put 3 bytes there.
        // We are using the first byte to mark the size of magic bytes.
        // It will end up with (02eeee).
        // If there is only
        //fprintf(stderr, "************size%x sz %x ptr %p, p %p nonAlignedBytes %d\n", size, sz, ptr, p, nonAlignedBytes);      
        if(setBytes >= 2) {
          //fprintf(stderr, "******setBytes %d\n", setBytes); 
          p[0] = setBytes - 1;
          for(int i = 1; i < setBytes; i++) {
            p[i] = xdefines::MAGIC_BYTE_NOT_ALIGNED;
          }
        }
        else if(setBytes == 1){
          //fprintf(stderr, "******setBytes %d\n", setBytes); 
          p[0] = xdefines::MAGIC_BYTE_NOT_ALIGNED;
        }

        sentinelmap::getInstance().markSentinelAt(startp);
//#endif
        // We actually setup a next word to capture the next word
        if(offset > xdefines::WORD_SIZE) {
          void * nextp = (void *)((intptr_t)p + setBytes);
          sentinelmap::getInstance().setSentinelAt(nextp);
        }
      }
    }
#endif
//    if((unsigned long)ptr < 0x100003000) {
//      fprintf(stderr, "DoubleTake: malloc ptr %p size %d mysize %d\n", ptr, sz, mysize);
//    }
    // We donot need to do anything if size is equal to sz
    //PRINF("malloc with sz %d ptr %p\n", sz, ptr);
    return ptr;
  }

  // We are trying to find the aligned address starting from ptr
  // to ptr+boundary.
  inline void * getAlignedAddress(void * ptr, size_t boundary) {
    return (void *)(((intptr_t)ptr + boundary) & (~(boundary - 1)));
  }

  inline void * memalign(size_t boundary, size_t sz) {
    // Actually, malloc is easy. Just have more memory at first.
    void * ptr = malloc(boundary + sz);

    // Since the returned ptr is not aligned, return next aligned address
    void * newptr = getAlignedAddress(ptr, boundary); 

    // check offset between newptr and ptr
    size_t offset = (intptr_t)newptr - (intptr_t)ptr;
    if(offset == 0) {
      newptr = (void *) ((intptr_t)newptr + boundary);
      offset = boundary;
    }

    // Check whether the offset is valid?
    assert(offset >= 2 * sizeof(size_t));
#ifdef DETECT_OVERFLOW
    // Put a sentinel before the this memory block.
    sentinelmap::getInstance().setMemalignSentinelAt((void *)((intptr_t)newptr - sizeof(size_t)));
#endif
    // Put the offset before the sentinel too
    void ** origptr = (void **)((intptr_t)newptr - 2 * sizeof(size_t)); 
    *origptr = ptr;

//    PRINF("offset is %x ptr %p and newptr %p\n", offset, ptr, newptr);
    return newptr;
  }

#ifdef DETECT_OVERFLOW
  bool isObjectOverflow(void * ptr) {
    bool isOverflow = false;


    // Check overflows for this object
    objectHeader * o = getObject (ptr);

    // Get the block size
    size_t size = o->getSize();

    // Set actual size there.
    size_t sz = o->getObjectSize();

    if(size < sz) {
      PRINF("Free isObjectOverflow size %x sz %x\n", size, sz);
      assert(size >= sz);
    }
    // Add another guard zone if block size is larger than actual size
    // in order to capture those 1 byte overflow.
    if(size > sz) {
      size_t offset = size - sz;

      // We are using the ptr to varify the size
      unsigned char * p = (unsigned char *)((intptr_t)ptr + sz);
      
      size_t nonAlignedBytes = sz & xdefines::WORD_SIZE_MASK;
      if(nonAlignedBytes == 0) {
        // This is the easist thing
        // check up whether the sentinel is intact or not.
        if(sentinelmap::getInstance().checkAndClearSentinel(p) != true) {
          // Add this address to watchpoint
          PRINF("xmemory: checkandclearsentinal now\n");
          //fprintf(stderr, "xmemory: checkandclearsentinal at line %d\n", __LINE__);
          watchpoint::getInstance().addWatchpoint(p, *((size_t*)p)); 
          isOverflow = true;
        }
      } 
//#ifdef DETECT_NONALIGNED_OVERFLOW
      else {
        // fprintf(stderr, "xmemory line %d: nonAlignedBytes %d\n", __LINE__, nonAlignedBytes);
        // For those less than one word access, maybe we do not care since memory block is 
        // always returned by 8bytes aligned or 16bytes aligned.
        // However, some weird test cases has this overflow. So we should detect this now.
        void * startp = (void *)((intptr_t)p - nonAlignedBytes);
        size_t setBytes = xdefines::WORD_SIZE - nonAlignedBytes; 
        //fprintf(stderr, "xmemory line %d: nonAlignedBytes %d startp %p setBytes %d\n", __LINE__, nonAlignedBytes, startp, setBytes);
        if(setBytes > 1 && (int)p[0] == (setBytes - 1)) {
          for(int i = 1; i < setBytes; i++) {
            if(p[i] != xdefines::MAGIC_BYTE_NOT_ALIGNED) {
              isOverflow = true;
              break;
            }
          }
        }
        else if(setBytes == 1) {
          if(p[0] != xdefines::MAGIC_BYTE_NOT_ALIGNED) 
            isOverflow = true;
        }
        else {
          isOverflow = true;
        }
      
        if(isOverflow) {
          //PRINF("xmemory: checkandclearsentinal now 222\n");
 //         fprintf(stderr, "xmemory: nonaligned byte ptr %p. size %d sz %d\n", ptr, size, sz);
          watchpoint::getInstance().addWatchpoint(startp, *((size_t *)startp)); 
        }
        sentinelmap::getInstance().clearSentinelAt(startp);
    
        // We actually setup a next word to capture the next word
        if(offset > xdefines::WORD_SIZE) {
          void * nextp = (void *)((intptr_t)p - nonAlignedBytes + xdefines::WORD_SIZE);
          if(sentinelmap::getInstance().checkAndClearSentinel(nextp) != true) {
            // Add this address to watchpoint
            watchpoint::getInstance().addWatchpoint(nextp, *((size_t *)nextp)); 
            isOverflow = true;
          }
        }
      }
// #endif 
    }  

    return isOverflow;
  }
#endif

  // Get the actual pointer of memory block
  // Since we are putting a special magic number before this pointer
  // we can check the magic number.
  // If the magic number is corrupted, then we can find out later.
  void * getObjectPtrAtFree(void * ptr) {
    size_t * prevPtr = (size_t *)((intptr_t)ptr - sizeof(size_t));
    void * origptr = ptr;

    if(*prevPtr == xdefines::MEMALIGN_SENTINEL_WORD) {
      void ** ppPtr = (void **)((intptr_t)ptr - 2 *sizeof(size_t));
#ifdef DETECT_OVERFLOW
      // Now we will cleanup the sentinel word.
      sentinelmap::getInstance().clearSentinelAt(prevPtr);
#endif
      origptr = *ppPtr;
    } 
    return origptr;
  }

  bool inRange(intptr_t addr) {
    return (addr > _heapBegin && addr < _heapEnd) ? true : false;
  }

  // We should mark this whole objects with 
  // some canary words.
  // Change the free operation to put into the tail of  
  // list.
   void free (void * ptr) {
    void * origptr;
   
   // fprintf(stderr, "DoubleTake, line %d: free ptr %p\n", __LINE__, ptr);
    if(!inRange((intptr_t)ptr)) {
      return;
    }

   // if((unsigned long)ptr < 0x100003000) {
   //   fprintf(stderr, "DoubleTake: free ptr %p\n", ptr);
   // }
    // Check whether this is an memaligned object.
    origptr = getObjectPtrAtFree(ptr);
    objectHeader * o = getObject (origptr);

    //fprintf(stderr, "DoubleTake, line %d: free ptr %p\n", __LINE__, ptr);
    		
#ifdef DETECT_OVERFLOW
    // Check for double free
    if(o->isObjectFree() || !o->isGoodObject()) {
      PRINF("Caught double free or invalid free error\n");
      fprintf(stderr, "DoubleTake: Caught double free or invalid free error\n");
      printCallsite();
      abort();
    }

    //printf("free ptr %p\n", ptr);
    // If this object has a overflow, we donot need to free this object
    if(isObjectOverflow(origptr)) {
      return;
    }
#else
    //fprintf(stderr, "DoubleTake, line %d: free ptr %p\n", __LINE__, ptr);
    if(o->isObjectFree() || !o->isGoodObject()) {
      return;
    }
#endif

    _pheap.free(origptr);
//    fprintf(stderr, "DoubleTake, line %d: free ptr %p\n", __LINE__, ptr);

    // We only remove the size 
    o->setObjectFree();
    // Cleanup this object with sentinel except the first word. 
  }


  /// @return the allocated size of a dynamically-allocated object.
  inline size_t getSize (void * ptr) {
    // Just pass the pointer along to the heap.
    return _pheap.getSize (ptr);
  }
  
  // Commit something without check the heap overflow
  void atomicCommit(void * addr, size_t size) {
    int index;
#if 0
    if (inRange (addr)) {
      _pheap.commit (addr, size);
    }
#endif
  }

  /// Called when a thread need to rollback.
  inline void rollback(void) {

    // Release all private pages.
    PRWRN("Recoverring the global memory\n");
    _globals.recoverMemory();
    _pheap.recoverMemory();
    
    _pheap.recoverHeapMetadata(); 
 
  //  fprintf(stderr, "INSTALL watching points nowwwwww!!!\n"); 
    // Now those watchpoints should be saved successfully,
    // We might have to install the watchpoints now.
    watchpoint::getInstance().installWatchpoints();
  //  PRWRN("Recoverring the global memory, after install watchpoints\n");
  }

  /// Rollback only without install watchpoints.
  inline void rollbackonly(void) {

    // Release all private pages.
    _globals.recoverMemory();
    _pheap.recoverMemory();
  
    // We should recover heap metadata in the end since 
    // we will pass the position of heap inside recoverMemory. 
    _pheap.recoverHeapMetadata(); 

    // We do not need to install watch points if we only rollback.
    watchpoint::getInstance().installWatchpoints();
 } 
  
  inline void printCallsite(void) {
    selfmap::getInstance().printCallStack(NULL, NULL, true);
    PRINF("Program exit becaue of double free or invalid free.\n");
    exit(-1);
  }

  /// Transaction begins.
  inline void epochBegin (void) {
    _pheap.saveHeapMetadata();

    // Backup all existing data. 
		_pheap.backup();
		_globals.backup();
  }

  inline void * getHeapEnd(void) {
    return _pheap.getHeapEnd();
  }

  inline void * getHeapBegin(void) {
    return (void *)_heapBegin;
  }

#ifdef DETECT_OVERFLOW
  // This function is called before the system call is issued.
  inline bool checkOverflowBeforehand(void * start, size_t size) {
    bool hasProblem = false;

    // If it is not at heap, we do not care.
    if (_pheap.inRange(start)) {
      // Now we will check whether the final position should be in 
      // the heap too.
      void * end = (void *)((intptr_t)start + size);
      if(_pheap.inRange(end)) {
        // check the possible overflow now.
        // We simply check whether this area has sentinels or not.
        // If there exists some sentinels there, it is a possible overflow.
        hasProblem = sentinelmap::getInstance().hasSentinels(start, size);
        PRINF("CAN NOT write to an area with sentinels\n");
      }
      else {
        hasProblem = true;
      }
    }

    return hasProblem;
  }
#endif

  // Check and commit in the end of transaction. 
  inline bool checkHeapOverflow() {
    bool hasOverflow = false; 

    // Whether it is a rollback phase
    if(global_isRollback()) {
      return false;
    }

 #ifdef DETECT_OVERFLOW 
    hasOverflow = _pheap.checkHeapOverflow();
 #endif
    //double elapse = stop(&startTime, NULL);
    if(hasOverflow == false) {
      // Check whether overflows and underflows has been detected
      // in the normal execution phase, like free()
      if(watchpoint::getInstance().hasToRollback()) {
        hasOverflow = true;
      }
    }
    return hasOverflow;
  }
 
  objectHeader * getObjectHeader (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

  static objectHeader * getObject (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

  void freeAllObjects(void);

  void realfree(void * ptr);
  void cleanupFreeList(void);

  /// Rollback to previous 
  static void handleSegFault();
  /* Signal-related functions for tracking page accesses. */

  /// @brief Signal handler to trap SEGVs.
  static void segvHandle (int signum, siginfo_t * siginfo, void * context) 
  {
    void * addr = siginfo->si_addr; // address of access

//    while(1) ;
    //PRDBG("%d: Segmentation fault error %d at addr %p!\n", current->index, siginfo->si_code, addr);
    fprintf(stderr, "Thread%d: Segmentation fault error %d at addr %p!\n", current->index, siginfo->si_code, addr);
    current->internalheap = true;
    selfmap::getInstance().printCallStack(NULL, NULL, true);
    current->internalheap = false;
    while(1);

//    WRAP(exit)(-1);
    // Set the context to handleSegFault
    jumpToFunction((ucontext_t *)context, (unsigned long)xmemory::getInstance().handleSegFault);   
//    xmemory::getInstance().handleSegFault ();
  }

  /// @brief Install a handler for SEGV signals.
  void installSignalHandler (void) {
#if defined(linux)
    static stack_t _sigstk;

    // Set up an alternate signal stack.
    _sigstk.ss_sp = MM::mmapAllocatePrivate (SIGSTKSZ);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    WRAP(sigaltstack)(&_sigstk, (stack_t *) 0);
#endif
    // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset (&siga.sa_mask);

    // Set the following signals to a set 
    sigaddset (&siga.sa_mask, SIGSEGV);

    WRAP(sigprocmask)(SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
#if defined(linux)
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;
#else
    siga.sa_flags = SA_SIGINFO | SA_RESTART;
#endif

    siga.sa_sigaction = xmemory::segvHandle;
    if (WRAP(sigaction)(SIGSEGV, &siga, NULL) == -1) {
      printf ("sfug.\n");
      exit (-1);
    }

    WRAP(sigprocmask) (SIG_UNBLOCK, &siga.sa_mask, NULL);
  }

private:

  /// The globals region.
  xglobals   _globals;

  intptr_t _heapBegin;
  intptr_t _heapEnd;

  /// The protected heap used to satisfy small objects requirement. Less than 256 bytes now.
  static xpheap<xoneheap<xheap > > _pheap;
};

#endif
