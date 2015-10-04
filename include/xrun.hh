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

#ifdef GET_CHARECTERISTICS
extern "C" {
	extern unsigned long count_epochs;
};
#endif
class xrun {

private:
  xrun()
      : _memory(xmemory::getInstance()), _thread(xthread::getInstance()),
        _watchpoint(watchpoint::getInstance())
  {
    // PRINF("xrun constructor\n");
  }

public:

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

    // if there is no limit for our stack size, then just pick a
    // reasonable limit.
    if (rl.rlim_cur == (rlim_t)-1) {
      rl.rlim_cur = 2048*4096; // 8 MB
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

    installSignalHandlers();

    // Initialize the internal heap at first.
    InternalHeap::getInstance().initialize();

    _thread.initialize();

    // Initialize the memory (install the memory handler)
    _memory.initialize();

    syscallsInitialize();
  }

  void finalize() {
#ifdef GET_CHARECTERISTICS
			fprintf(stderr, "DOUBLETAKE has epochs %ld\n", count_epochs);
#endif
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

  int getThreadIndex() const { return _thread.getThreadIndex(); }
  char *getCurrentThreadBuffer() { return _thread.getCurrentThreadBuffer(); }

private:
  void syscallsInitialize();
  void stopAllThreads();

  // Handling the signal SIGUSR2
  static void sigusr2Handler(int signum, siginfo_t* siginfo, void* uctx);
  static void sigsegvHandler(int signum, siginfo_t *siginfo, void *uctx);

  void installSignalHandlers();

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
};

#endif
