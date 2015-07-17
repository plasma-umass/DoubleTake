/*
 * @file   watchpoint.cpp
 * @brief  Including the implemenation of watch point handler.
 *         Since we have to call functions in other header files, then it is
 *         impossible to put this function into the header file.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "watchpoint.hh"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syscall.h>
#include <ucontext.h>
#include <unistd.h>

#include "log.hh"
#include "memtrack.hh"
#include "real.hh"
#include "selfmap.hh"
#include "xdefines.hh"

long perf_event_open(struct perf_event_attr* hw_event, pid_t pid, int cpu, int group_fd,
                     unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}
#if 0
pid_t gettid() {
  return syscall(__NR_gettid);
}
#endif

bool watchpoint::addWatchpoint(void* addr, size_t value, faultyObjectType objtype,
                               void* objectstart, size_t objectsize) {
  bool hasWatchpoint = true;

#ifndef EVALUATING_PERF
  if(objtype == OBJECT_TYPE_OVERFLOW) {
    PRINT("DoubleTake: Buffer overflow at address %p with value 0x%lx. size %lx start %p\n",
	  addr, value, objectsize, objectstart);
    // PRINT("DoubleTake: Buffer overflow at address %p with value 0x%lx. \n", addr, value);
  } 
	else if(objtype == OBJECT_TYPE_USEAFTERFREE) {
   // assert(objtype == OBJECT_TYPE_USEAFTERFREE);
    PRINT("DoubleTake: Use-after-free error detected at address %p.", addr);
  }
#endif

  if(_numWatchpoints < xdefines::MAX_WATCHPOINTS) {
    // Record watch point information
    _wp[_numWatchpoints].faultyaddr = addr;
    _wp[_numWatchpoints].objectstart = objectstart;
    _wp[_numWatchpoints].objtype = objtype;
    //  _wp[_numWatchpoints].objectsize = objectsize;
    _wp[_numWatchpoints].faultyvalue = value;
    _wp[_numWatchpoints].currentvalue = value;
    _numWatchpoints++;
  } else {
    hasWatchpoint = false;
  }

  // Add it to memtrack too.
	if(objtype != OBJECT_TYPE_WATCHONLY)
  	memtrack::getInstance().insert(objectstart, objectsize, objtype);

  return !hasWatchpoint;
}

bool watchpoint::findFaultyObject(faultyObject** object) {
  int trigPoints = 0;

  PRINF("findFaultyObject: _numWatchpoints %d\n", _numWatchpoints);
  for(int i = 0; i < _numWatchpoints; i++) {
    unsigned long value = *((unsigned long*)_wp[i].faultyaddr);
#ifndef EVALUATING_PERF
    	PRINT("%d point: address %p currentvalue %lx value %lx\n", trigPoints, _wp[i].faultyaddr, _wp[i].currentvalue, value);
#endif
    // Check whether now overflow actually happens
    if(value != _wp[i].currentvalue) {
      //				PRINT("WARNING: we %d points, currentvalue %lx value %lx\n", trigPoints,
      //_wp[i].currentvalue, value);
      _wp[i].currentvalue = value;
      *object = &_wp[i];
      trigPoints++;
      continue;
    }
  }

  if(trigPoints > 1) {
    PRINT("WARNING: we have %d watchpoints triggered, only one watchpoint allowed.\n", trigPoints);
  }

  return (trigPoints != 0);
}

bool watchpoint::hasToRollback() { return _numWatchpoints > 0; }

void watchpoint::installWatchpoints() {
  struct sigaction trap_action;

  //  PRINT("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);
  // We don't need to setup watchpoints if it is 0.
  if(_numWatchpoints == 0) {
    return;
  }

  // Now we are setting a trap handler for myself.
  trap_action.sa_sigaction = watchpoint::trapHandler;
  trap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
  Real::sigaction(SIGTRAP, &trap_action, NULL);

  // Setup the watchpoints information and notify the daemon (my parent)
  struct watchpointsInfo wpinfo;
  wpinfo.count = _numWatchpoints;
  int perffd;

  //  PRINT("Install watchpoints: _numWatchpoints %d\n", _numWatchpoints);
  // Get the initial value of different watchpoints.
  for(int i = 0; i < _numWatchpoints; i++) {
    // PRINT("Watchpoint %d: addr %p. _numWatchpoints %d\n", i, _wp[i].faultyaddr, _numWatchpoints);
    wpinfo.wp[i] = (unsigned long)_wp[i].faultyaddr;
    // PRINT("Watchpoint %d: addr %p\n", i, _wp[i].faultyaddr);
    // Update the values of those faulty address so that
    // we can compare those values to find out which watchpoint
    // are accessed since we don't want to check the debug status register
    _wp[i].currentvalue = *((unsigned long*)_wp[i].faultyaddr);

    // install this watch point.
    perffd = install_watchpoint((uintptr_t)_wp[i].faultyaddr, SIGTRAP, -1);
    // PRINT("Watchpoint %d: addr %p done\n", i, _wp[i].faultyaddr);
  }

  // Now we can start those watchpoints.
  enable_watchpoints(perffd);
  // We actually don't care about what content.
}

int watchpoint::install_watchpoint(uintptr_t address, int sig, int group) {
  // Perf event settings
  struct perf_event_attr pe = {.type = PERF_TYPE_BREAKPOINT,
                               .size = sizeof(struct perf_event_attr),
                               .bp_type = HW_BREAKPOINT_W,
                               .bp_len = HW_BREAKPOINT_LEN_4,
                               .bp_addr = (uintptr_t)address,
                               .disabled = 1,
                               .sample_period = 1, };

  // Create the perf_event for this thread on all CPUs with no event group
  int perf_fd = perf_event_open(&pe, 0, -1, group, 0);
  if(perf_fd == -1) {
    PRINT("Failed to open perf event file: %s\n", strerror(errno));
    abort();
  }
  // PRINT("Install watchpoint. perf_fd %d group %d\n", perf_fd, group);

  // Set the perf_event file to async mode
  if(Real::fcntl(perf_fd, F_SETFL, Real::fcntl(perf_fd, F_GETFL, 0) | O_ASYNC) == -1) {
    PRINT("Failed to set perf event file to ASYNC mode: %s\n", strerror(errno));
    abort();
  }

  // Tell the file to send a SIGUSR1 when an event occurs
  if(Real::fcntl(perf_fd, F_SETSIG, sig) == -1) {
    PRINT("Failed to set perf event file's async signal: %s\n", strerror(errno));
    abort();
  }

  // Deliver the signal to this thread
  //   if(Real::fcntl(perf_fd, F_SETOWN, syscall(__NR_gettid)) == -1) {
  if(Real::fcntl(perf_fd, F_SETOWN, getpid()) == -1) {
    fprintf(stderr, "Failed to set the owner of the perf event file: %s\n", strerror(errno));
    abort();
  }

  return perf_fd;
}

void watchpoint::enable_watchpoints(int fd) {
  // Start the event
  if(ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
    fprintf(stderr, "Failed to enable perf event: %s\n", strerror(errno));
    abort();
  }
}

void watchpoint::disable_watchpoint(int fd) {
  // Start the event
  if(ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
    fprintf(stderr, "Failed to disable perf event: %s\n", strerror(errno));
    abort();
  }
}

bool watchpoint::checkAndSaveCallsite(faultyObject* object, int depth, void** callsite) {
  return object->faultySite.saveAndCheck(depth, callsite);
}

// Handle those traps on watchpoints now.
void watchpoint::trapHandler(int /* sig */, siginfo_t* /* siginfo */, void* context) {
  ucontext_t* trapcontext = (ucontext_t*)context;
  size_t* addr = (size_t*)trapcontext->uc_mcontext.gregs[REG_RIP]; // address of access

  // Find faulty object.
  faultyObject* object;

  PRINT("inside the trap handler, address %p with value %lx\n", addr, *((unsigned long *)addr));
  // If it is a read, we only care about this if it is use-after-free error
  if(!watchpoint::getInstance().findFaultyObject(&object)) {
    PRERR("Can't find faulty object!!!!\n");
    return;
  }

  // Check whether this trap is caused by libdoubletake library.
  // If yes, then we don't care it since libdoubletake can fill the canaries.
  if(selfmap::getInstance().isDoubleTakeLibrary(addr)) {
    return;
  }

  //  PRINF("CAPTURING write at %p: ip %lx. signal pointer %p, code %d. \n", addr,
  // trapcontext->uc_mcontext.gregs[REG_RIP], siginfo->si_ptr, siginfo->si_code);
  faultyObjectType faultType;
	if(object->objtype != OBJECT_TYPE_WATCHONLY) {
  	faultType = memtrack::getInstance().getFaultType(object->objectstart, object->faultyaddr);
  	if(faultType == OBJECT_TYPE_NO_ERROR) {
    	return;
  	}
	}
	else {
    PRINT("\nWatch a memory access on %p (value %lx) with call stack:\n", object->faultyaddr, *((unsigned long *)object->faultyaddr));
  	selfmap::getInstance().printCallStack();
		return;
	}

  // Check whether this callsite is the same as the previous callsite.
  void* callsites[xdefines::CALLSITE_MAXIMUM_LENGTH];
  int depth = selfmap::getCallStack((void**)&callsites);

  // If current callsite is the same as the previous one, we do not want to report again.
  if(watchpoint::getInstance().checkAndSaveCallsite(object, depth, (void**)&callsites)) {
    return;
  }

  // Now we should check whether objectstart is existing or not.
  if(faultType == OBJECT_TYPE_OVERFLOW) {
    PRINT("\nCaught a heap overflow at %p. Current call stack:\n", object->faultyaddr);
  } else if(faultType == OBJECT_TYPE_USEAFTERFREE) {
    PRINT("\nCaught a use-after-free error at %p. Current call stack:\n", object->faultyaddr);
  }
  selfmap::getInstance().printCallStack(depth, (void**)&callsites);

  // Check its allocation or deallocation inf
  if(object->objectstart != NULL) {
    // Now we should check memtrack status.
    memtrack::getInstance().print(object->objectstart, faultType);
  }
}
