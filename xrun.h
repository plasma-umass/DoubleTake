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
 * @file   xrun.h
 * @brief  The main engine for consistency management, etc.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _XRUN_H_
#define _XRUN_H_

#include "xdefines.h"

// memory
#include "internalheap.h"
#include "xmemory.h"

// Grace utilities
#include "atomic.h"

#include "libfuncs.h"

#include "xthread.h"

class xrun {

private:

  xrun (void)
  : 
    _hasRollbacked(false), 
    _memory (xmemory::getInstance()),
    _thread (xthread::getInstance()),
    _watchpoint (watchpoint::getInstance())
  {
  //PRINF("xrun constructor\n");
  }

public:

  static xrun& getInstance (void) {
    static char buf[sizeof(xrun)];
    static xrun * theOneTrueObject = new (buf) xrun();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize (void)
  {
    //current = NULL;
    pid_t pid = syscall(SYS_getpid);
    struct rlimit rl;

    // Get the stack size.
    if (WRAP(getrlimit)(RLIMIT_STACK, &rl) != 0) {
      PRINF("Get the stack size failed.\n");
      WRAP(exit)(-1);
    }
    
    // Check the stack size.
    __max_stack_size = rl.rlim_cur;
    //fprintf(stderr, "starting max_stacksize %lx!!!!!\n", __max_stack_size);
#if 0 
    rl.rlim_cur = 524288;
    rl.rlim_max = 1048576;
    if(WRAP(setrlimit)(RLIMIT_NOFILE, &rl)) {
      fprintf(stderr, "change limit failed, error %s\n", strerror(errno));
    }
    fprintf(stderr, "NUMBER files limit %d\n", rl.rlim_cur);

    while(1);
#endif
    global_initialize();

    installSignalHandler();
    
  //  fprintf(stderr, "xrun::initialize line %d\n", __LINE__);
    InternalHeap::getInstance().initialize();

    // Initialize the internal heap at first.
    //InternalHeap::getInstance().malloc(8);
    _thread.initialize();
      
    // Initialize the memory (install the memory handler)
    _memory.initialize();

    _watchpoint.initialize();

    syscallsInitialize();

    // Set the current _tid to our process id.
    _pid = pid;
    _main_id = pid;

//    PRDBG("starting!!!!!\n");
    epochBegin();
    fprintf(stderr, "starting!!!!!\n");
  }
  
  void finalize (void)
  {
    if(mainId() != getpid()) {
      return;
    }

    //fprintf(stderr, "In the end of finalize function\n");
    //PRINF("%d: finalize now !!!!!\n", getpid());
    // If we are not in rollback phase, then we should check buffer overflow.
    if(!global_isRollback())
      epochEnd();

    PRINF("%d: finalize now !!!!!\n", getpid());
    // Now we have to cleanup all semaphores.
    _thread.finalize();
    
  }

  // Simply commit specified memory block
  void atomicCommit(void * addr, size_t size) {
    _memory.atomicCommit(addr, size);
  }

  int mainId() {
    return _main_id;
  }

  /* Transaction-related functions. */
  void saveContext(void) {
    _thread.saveContext();
  }

  /// Rollback to previous saved point 
  void rollback(void);
   
  /// Rollback to previous 
  void rollbackandstop(void);

  inline void setpid(int pid) {
    _pid = pid;
  }
 
  /// @ Return current pid.
  inline int getpid(void) {
    return _pid;
  }

  void epochBegin (void);
  void epochEnd (void);
private:
  void syscallsInitialize(void);
  void stopAllThreads(void);


  // Handling the signal SIGUSR2
  static void sigusr2Handler(int signum, siginfo_t * siginfo, void * context); 

  /// @brief Install a handler for SIGUSR2 signals.
  /// We are using the SIGUSR2 to stop all other threads.
  void installSignalHandler (void) {
    struct sigaction sigusr2;

    static stack_t _sigstk;

    // Set up an alternate signal stack.
    _sigstk.ss_sp = MM::mmapAllocatePrivate ( SIGSTKSZ);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    WRAP(sigaltstack)(&_sigstk, (stack_t *) 0);

    // We don't want to receive SIGUSR2 again when a thread is inside signal handler.
    sigemptyset (&sigusr2.sa_mask);
    sigaddset(&sigusr2.sa_mask, SIGUSR2);
  //  WRAP(sigprocmask) (SIG_BLOCK, &sigusr2.sa_mask, NULL);
    /** 
      Some parameters used here:
       SA_RESTART: Provide behaviour compatible with BSD signal 
                   semantics by making certain system calls restartable across signals.
       SA_SIGINFO: The  signal handler takes 3 arguments, not one.  In this case, sa_sigac-
                   tion should be set instead of sa_handler.
                   So, we can acquire the user context inside the signal handler 
    */
    sigusr2.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;

    sigusr2.sa_sigaction = xrun::sigusr2Handler;
    if (WRAP(sigaction)(SIGUSR2, &sigusr2, NULL) == -1) {
      fprintf (stderr, "setting signal handler SIGUSR2 failed.\n");
      EXIT;
    }
  }


  // Notify the system call handler about rollback phase
  void startRollback();

  xthread&  _thread;
  /// The memory manager (for both heap and globals).
  xmemory&     _memory;
  watchpoint&     _watchpoint;

  volatile  bool   _hasRollbacked;
 
//  int   _rollbackStatus; 
  int   _pid; //The first process's id.
  int   _main_id;
};


#endif
