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

  void initialize();
  void finalize();

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
  static void rollbackFromSegv();

  void endOfEpochSignal(ucontext_t *uctx);
  void rollbackFromSegvSignal();

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
