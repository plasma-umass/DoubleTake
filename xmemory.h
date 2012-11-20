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
#include "xrun.h"

// Heap Layers
#include "privateheap.h"
#include "stlallocator.h"
//#include "warpheap.h"
#include "xpheap.h"

#include "sourcesharedheap.h"
#include "internalheap.h"
#include "xoneheap.h"
#include "xheap.h"

#include "xplock.h"
#include "xpageentry.h"
#include "xpagestore.h"
#include "objectheader.h"

#include "watchpoint.h"

// Encapsulates all memory spaces (globals & heap).

class xmemory {
private:

  // Private on purpose. See getInstance(), below.
  xmemory (void) 
   : _internalheap (InternalHeap::getInstance())
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
    // Install a handler to ntercept SEGV signals (used for trapping initial reads and
    // writes to pages).
    installSignalHandler();
	
    // Call _pheap so that xheap.h can be initialized at first and then can work normally.
    _pheap.initialize();
	  _globals.initialize();
	  xpageentry::getInstance().initialize();
	  xpagestore::getInstance().initialize();

    // Now we can initialize the part about sanity checking.
    // Why we are doing this because we try to avoid the checking 
    // on the metadata of heaps.
    _pheap.sanitycheckInitialize();
  }

  void finalize(void) {
	  _globals.finalize();
	  _pheap.finalize();
  }

  // Intercepting those allocations
  inline void *malloc (size_t sz) {
  	void * ptr;
  
    ptr = _pheap.malloc (_heapid, sz);
 
    //printf("malloc with sz %d ptr %p\n", sz, ptr);
    return ptr;
  }


  inline void * realloc(void * ptr, size_t sz) {
    size_t s = getSize (ptr);

    void * newptr =  malloc(sz);
    if (newptr && s != 0) {
      size_t copySz = (s < sz) ? s : sz;
      memcpy (newptr, ptr, copySz);
    }

	  free(ptr);
	  return newptr;
  }

  // We should mark this whole objects with 
  // some canary words.
  // Change the free operation to put into the tail of  
  // list.
  inline void free (void * ptr) {
    size_t s = getSize (ptr);
		_pheap.free(_heapid, ptr);

    // Cleanup this object with sentinel except the first word. 
  }

  /// @return the allocated size of a dynamically-allocated object.
  inline size_t getSize (void * ptr) {
    // Just pass the pointer along to the heap.
    return _pheap.getSize (ptr);
  }
 
  void openProtection(void) {
   // fprintf(stderr, "open Protection\n");
    _globals.openProtection();
    _pheap.openProtection();
  }

  void closeProtection(void) {
    // memory spaces (globals and heap).
    _globals.closeProtection();
    _pheap.closeProtection();
  }

  /// Called when a thread need to rollback.
  inline void rollback(void) {
    // Now those watchpoints should be saved successfully,
    // We might have to install the watchpoints now.
    watchpoint::getInstance().installWatchpoints();

    // Save the heapmeta data.
    _pheap.recoverHeapMetadata(); 

    // Release all private pages.
    _globals.begin();
    _pheap.begin();
  }

  /// Transaction begins.
  inline void atomicBegin (void) {
    // Reset global and heap protection.
    _globals.begin();
    _pheap.begin();

    // Save the heapmeta data.
    _pheap.saveHeapMetadata(); 
  }

  inline void handleWrite (void * addr) {
    int index;
    if (_pheap.inRange (addr)) {
      _pheap.handleWrite (addr);
    }
    else if (_globals.inRange (addr, &index)) {
      _globals.handleWrite (addr, index);
    }
    // None of the above - something is wrong.
#ifndef NDEBUG
    else { 
      printf ("out of range!\n");
    }
#endif
  }

  // Check and commit in the end of transaction.  
  // We are using two phases here:
  // First, we will check whether there is some overflow error or not.
  // Second, if there is no overflow, then we can start to commit the
  // changes.
  inline bool atomicEnd (void) {
    bool hasOverflow = false; 
    // Check whether there are some overflow here.
    //if(_pheap.sanitycheckPerform(&watchpoints[0]) == false) {
    hasOverflow = _pheap.sanitycheckPerform();
    if(hasOverflow == false) {
	  	fprintf(stderr, "%d : atomic commit, NO OVERFLOW\n", getpid());
      // Commit if there is no overflow.
		  _pheap.commit();
		  _globals.commit();
    }
    else {
	  	fprintf(stderr, "%d : OVERFLOW!!!!\n", getpid());

    }
    return hasOverflow;
  }

  // We will check the overflow in the end.
  // There is no need to commit those changes,
  // we only need to check whether there is some overflow.
  // If no overflow, then we don't need to do anything.
  // If some overflows, then we have to rollback.
  inline void programEnd(void) {
    bool hasOverflow = false; 
	//	fprintf(stderr, "%d : atomic commit\n", getpid());
    // Check whether there are some overflow here.
    //if(_pheap.sanitycheckPerform(&watchpoints[0]) == false) {
    hasOverflow = _pheap.sanitycheckPerform();
    
  }

#if 0
  inline void write_lock (void) {
	  _lock.lock();
  }

  inline void unlock (void) {	_lock.unlock(); }
#endif

  unsigned long sharemem_read_word(void * dest) {
    int index;
    if(_pheap.inRange(dest)) {
      return _pheap.sharemem_read_word(dest);
    } 
    else if(_globals.inRange(dest, &index)) {
      return _globals.sharemem_read_word(dest, index);
    } 
#ifndef NDEBUG
    else { 
      printf ("Read a word from %p, but it is out of range!\n", dest);
    }
#endif
  }

  void sharemem_write_word(void * dest, unsigned long val) {
    int index;
    if(_pheap.inRange(dest)) {
      _pheap.sharemem_write_word(dest, val);
    } 
    else if(_globals.inRange(dest, &index)) {
      _globals.sharemem_write_word(dest, val, index);
    }
#ifndef NDEBUG
    else { 
      printf ("Write a word at %p, but it is out of range!\n", dest);
    }
#endif
  }

  void setHeapId(int index) {
    _heapid = index;
  }

  inline int setThreadIndex(int heapid) {
    //fprintf(stderr, "%d: setheapid %d\n", getpid(), heapid);
    _heapid = heapid%xdefines::NUM_HEAPS;
  }


private:
  objectHeader * getObjectHeader (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

public:

  /* Signal-related functions for tracking page accesses. */

  /// @brief Signal handler to trap SEGVs.
  static void segvHandle (int signum, siginfo_t * siginfo, void * context) 
  {
    void * addr = siginfo->si_addr; // address of access

    // Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
      // Compute the page that holds this address.
      void * page = (void *) (((size_t) addr) & ~(xdefines::PageSize-1));

      // Unprotect the page and record the write.
      mprotect ((char *) page,
                xdefines::PageSize,
                PROT_READ | PROT_WRITE);

	    // It is a write operation. Handle that.
      xmemory::getInstance().handleWrite (addr);
    } else if (siginfo->si_code == SEGV_MAPERR) {
      fprintf (stderr, "%d : map error with addr %p!\n", getpid(), addr);
      ::abort();
    } else {

      fprintf (stderr, "%d : other access error with addr %p.\n", getpid(), addr);
      ::abort();
    }
  }

  /// @brief Install a handler for SEGV signals.
  void installSignalHandler (void) {
#if defined(linux)
    static stack_t _sigstk;

    // Set up an alternate signal stack.
    _sigstk.ss_sp = MM::mmapAllocatePrivate ( SIGSTKSZ, -1);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    sigaltstack (&_sigstk, (stack_t *) 0);
#endif
    // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    sigemptyset (&siga.sa_mask);

    // Set the following signals to a set 
    sigaddset (&siga.sa_mask, SIGSEGV);

    sigprocmask (SIG_BLOCK, &siga.sa_mask, NULL);

    // Point to the handler function.
#if defined(linux)
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_NODEFER;
#else
    siga.sa_flags = SA_SIGINFO | SA_RESTART;
#endif

    siga.sa_sigaction = xmemory::segvHandle;
    if (sigaction (SIGSEGV, &siga, NULL) == -1) {
      printf ("sfug.\n");
      exit (-1);
    }

    sigprocmask (SIG_UNBLOCK, &siga.sa_mask, NULL);
  }

private:

  /// The globals region.
  xglobals   _globals;

  /// The protected heap used to satisfy small objects requirement. Less than 256 bytes now.
#if 0  
  warpheap<xdefines::NUM_HEAPS, xdefines::PROTECTEDHEAP_CHUNK, xoneheap<xheap<xdefines::PROTECTEDHEAP_SIZE> > > _pheap;
#endif
  //xpheap<xoneheap<xheap<xdefines::PROTECTEDHEAP_SIZE> > > _pheap;
  xpheap<xoneheap<xheap > > _pheap;
 
  
//  warpheap<xdefines::NUM_HEAPS, xdefines::SHAREDHEAP_CHUNK, 
//	xoneheap<SourceSharedHeap<xdefines::SHAREDHEAP_SIZE> > > _sheap;

  typedef std::set<void *, less<void *>,
		   HL::STLAllocator<void *, PrivateHeap> > // myHeap> >
  pagesetType;

  /// Internal share heap.
  InternalHeap 		_internalheap;

  /// A signal stack, for catching signals.

  /// A lock that protect the global area.
  xplock _lock;
  int _heapid;
};

#endif
