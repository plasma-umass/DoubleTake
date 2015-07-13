#if !defined(DOUBLETAKE_WATCHPOINT_H)
#define DOUBLETAKE_WATCHPOINT_H

/*
 * @file   watchpoint.h
 * @brief  Watch point handler, we are relying the dr.c to add watchpoint and detect the condition
 * of
 *         watch point, dr.c is adopted from GDB-7.5/gdb/i386-nat.c.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#include <new>

#include "callsite.hh"
#include "xdefines.hh"

class watchpoint {
public:
  class faultyObject {
  public:
    faultyObjectType objtype;
    void* faultyaddr;
    void* objectstart;
    unsigned long faultyvalue;
    unsigned long currentvalue;
    CallSite faultySite;
  };

  static watchpoint& getInstance() {
    static char buf[sizeof(watchpoint)];
    static watchpoint* theOneTrueObject = new (buf) watchpoint();
    return *theOneTrueObject;
  }

  // Add a watch point with its value to watchpoint list.
  bool addWatchpoint(void* addr, size_t value, faultyObjectType objtype, void* objectstart,
                     size_t objectsize);

  bool findFaultyObject(faultyObject** object);

  bool hasToRollback();

  // Set all watch points before rollback.
  void installWatchpoints();

  // Use perf_event_open to install a particular watch points.
  int install_watchpoint(uintptr_t address, int sig, int group);

  // Enable a setof watch points now
  void enable_watchpoints(int fd);

  void disable_watchpoint(int fd);

  // How many watchpoints that we should care about.
  int getWatchpointsNumber() { return _numWatchpoints; }

  bool checkAndSaveCallsite(faultyObject* object, int depth, void** callsite);

  // Handle those traps on watchpoints now.
  static void trapHandler(int sig, siginfo_t* siginfo, void* context);

private:
  watchpoint() : _numWatchpoints(0) {}
  ~watchpoint() {}

  int _numWatchpoints;

  // Watchpoint array, we can only support 4 watchpoints totally.
  faultyObject _wp[xdefines::MAX_WATCHPOINTS];
};

#endif
