#if !defined(DOUBLETAKE_XRUN_H)
#define DOUBLETAKE_XRUN_H

/*
 * @file   xrun.h
 * @brief  The main engine for consistency management, etc.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <unistd.h>

#include <new>

#include "globalinfo.hh"
#include "internalheap.hh"
#include "log.hh"
#include "mm.hh"
#include "real.hh"
#include "watchpoint.hh"
#include "xdefines.hh"
#include "xmemory.hh"
#include "xthread.hh"

class xrun {

private:
  xrun()
      : _memory(xmemory::getInstance()), _thread(xthread::getInstance()),
        _watchpoint(watchpoint::getInstance()),
	_detectOverflow (true),    // FIXME to be set from command-line
	_detectMemoryLeaks (false), // FIXME
	_detectUseAfterFree (true)
  {
    // PRINF("xrun constructor\n");
  }

public:

  bool detectOverflow() const     { return _detectOverflow; }
  bool detectMemoryLeaks() const  { return _detectMemoryLeaks; }
  bool detectUseAfterFree() const { return _detectUseAfterFree; }


  static xrun& getInstance() {
    static char buf[sizeof(xrun)];
    static xrun* theOneTrueObject = new (buf) xrun();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize() {
    //		PRINT("xrun: initialization at line %d\n", __LINE__);
    struct rlimit rl;

    // Get the stack size.
    if(Real::getrlimit(RLIMIT_STACK, &rl) != 0) {
      PRWRN("Get the stack size failed.\n");
      Real::exit(-1);
    }

    // Check the stack size.
    __max_stack_size = rl.rlim_cur;
#if 0 
    rl.rlim_cur = 524288;
    rl.rlim_max = 1048576;
    if(Real::setrlimit(RLIMIT_NOFILE, &rl)) {
      PRINF("change limit failed, error %s\n", strerror(errno));
    }
    PRINF("NUMBER files limit %d\n", rl.rlim_cur);

    while(1);
#endif

		// Initialize the locks and condvar used in epoch switches
    global_initialize();

    installSignalHandler();

    // Initialize the internal heap at first.
    InternalHeap::getInstance().initialize();

    _thread.initialize();

    // Initialize the memory (install the memory handler)
    _memory.initialize();

    syscallsInitialize();

  //  epochBegin();
    //    PRINT("starting!!!!!\n");

    //    PRINF("starting!!!!!\n");
  }

  void finalize() {
//    PRINT("%d: finalize now !!!!!\n", getpid());
    // If we are not in rollback phase, then we should check buffer overflow.
    if(!global_isRollback()) {
#ifdef DETECT_USAGE_AFTER_FREE
      finalUAFCheck();
#endif

      epochEnd(true);
    }

    //    PRINF("%d: finalize now !!!!!\n", getpid());
    // Now we have to cleanup all semaphores.
    _thread.finalize();
  }

#ifdef DETECT_USAGE_AFTER_FREE
  void finalUAFCheck();
#endif
  // Simply commit specified memory block
  void atomicCommit(void* addr, size_t size) { _memory.atomicCommit(addr, size); }

  /* Transaction-related functions. */
  void saveContext() { _thread.saveContext(); }

  /// Rollback to previous saved point
  void rollback();

  /// Rollback to previous
  void rollbackandstop();

  void epochBegin();
  void epochEnd(bool endOfProgram);

private:
  void syscallsInitialize();
  void stopAllThreads();

  // Handling the signal SIGUSR2
  static void sigusr2Handler(int signum, siginfo_t* siginfo, void* context);

  /// @brief Install a handler for SIGUSR2 signals.
  /// We are using the SIGUSR2 to stop all other threads.
  void installSignalHandler() {
    struct sigaction sigusr2;

    static stack_t _sigstk;

    // Set up an alternate signal stack.
    _sigstk.ss_sp = MM::mmapAllocatePrivate(SIGSTKSZ);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    Real::sigaltstack(&_sigstk, (stack_t*)0);

    // We don't want to receive SIGUSR2 again when a thread is inside signal handler.
    sigemptyset(&sigusr2.sa_mask);
    sigaddset(&sigusr2.sa_mask, SIGUSR2);
    //  Real::sigprocmask (SIG_BLOCK, &sigusr2.sa_mask, NULL);
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
    if(Real::sigaction(SIGUSR2, &sigusr2, NULL) == -1) {
      fprintf(stderr, "setting signal handler SIGUSR2 failed.\n");
      abort();
    }
  }

  // Notify the system call handler about rollback phase
  void startRollback();

  /*  volatile bool _hasRolledBack; */

  /// The memory manager (for both heap and globals).
  xmemory& _memory;
  xthread& _thread;
  watchpoint& _watchpoint;


  //  int   _rollbackStatus;
  /*  int _pid; // The first process's id. */
  /* int _main_id; */

  const bool _detectOverflow;
  const bool _detectMemoryLeaks;
  const bool _detectUseAfterFree;
};

#endif
