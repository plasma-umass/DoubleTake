#if !defined(DOUBLETAKE_WATCHPOINT_H)
#define DOUBLETAKE_WATCHPOINT_H

/*
 * @file   watchpoint.h
 * @brief  Watch point handler, we are relying the dr.c to add watchpoint and detect the condition of 
 *         watch point, dr.c is adopted from GDB-7.5/gdb/i386-nat.c.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <signal.h>
#include <syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <execinfo.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <ucontext.h>
#include <asm/unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>

#include "xdefines.h"
#include "real.h"
#include "selfmap.h"
#include "callsite.h"

extern "C" {
extern long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);
};

class watchpoint {
  watchpoint() {
    _numWatchpoints = 0;
  }

  ~watchpoint() {
  }

public:

  class faultyObject {
    public:
      faultyObjectType objtype;
      void   * faultyaddr;
      void   * objectstart;
      unsigned long faultyvalue;
			unsigned long currentvalue;
			CallSite  faultySite;
  };
      
  static watchpoint& getInstance() {
    static char buf[sizeof(watchpoint)];
    static watchpoint * theOneTrueObject = new (buf) watchpoint();
    return *theOneTrueObject;
  }

  void initialize() {
		// Currently, we do nothing.
  }

  // Add a watch point with its value to watchpoint list.
  bool addWatchpoint(void * addr, size_t value, faultyObjectType objtype, void * objectstart, size_t objectsize);

  bool findFaultyObject(faultyObject ** object) {
		int trigPoints = 0;
		
//		PRINT("findFaultyObject: _numWatchpoints %d\n", _numWatchpoints);
    for(int i = 0; i < _numWatchpoints; i++) {
      unsigned long value = *((unsigned long *)_wp[i].faultyaddr);

      // Check whether now overflow actually happens
      if(value != _wp[i].currentvalue) {
//				PRINT("WARNING: we %d points, currentvalue %lx value %lx\n", trigPoints, _wp[i].currentvalue, value);
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

  bool hasToRollback() {
    return (_numWatchpoints > 0 ? true : false);
  }

  // Set all watch points before rollback. 
  void installWatchpoints()
  {
    struct sigaction trap_action;

    PRINT("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
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

		PRINT("Install watchpoints: _numWatchpoints %d\n", _numWatchpoints);
		// Get the initial value of different watchpoints.
		for(int i = 0; i < _numWatchpoints; i++) {
			//PRINT("Watchpoint %d: addr %p. _numWatchpoints %d\n", i, _wp[i].faultyaddr, _numWatchpoints);
			wpinfo.wp[i] = (unsigned long)_wp[i].faultyaddr;
			PRINT("Watchpoint %d: addr %p\n", i, _wp[i].faultyaddr);
			// Update the values of those faulty address so that 
			// we can compare those values to find out which watchpoint 
			// are accessed since we don't want to check the debug status register
			_wp[i].currentvalue = *((unsigned long *)_wp[i].faultyaddr);

			// install this watch point. 
			if(i == 0) {
				perffd = install_watchpoint((uintptr_t)_wp[i].faultyaddr, SIGTRAP, -1);
			}
			else {
				install_watchpoint((uintptr_t)_wp[i].faultyaddr, SIGTRAP, perffd);
			}
			PRINT("Watchpoint %d: addr %p done\n", i, _wp[i].faultyaddr);
		}	 

		// Now we can start those watchpoints.
  	enable_watchpoints(perffd);
		// We actually don't care about what content.
  }

	// Use perf_event_open to install a particular watch points.
  int install_watchpoint(uintptr_t address, int sig, int group) {
    // Perf event settings
    struct perf_event_attr pe = {
      .type = PERF_TYPE_BREAKPOINT,
      .size = sizeof(struct perf_event_attr),
      .bp_type = HW_BREAKPOINT_W,
      .bp_len = HW_BREAKPOINT_LEN_4,
      .bp_addr = (uintptr_t)address,
      .disabled = 1,
      .sample_period = 1,
    };
  
//		PRINT("Install watchpoing at line %d\n", __LINE__); 
    /*
    int perf_event_open(struct perf_event_attr *attr,
                             pid_t pid, int cpu, int group_fd,
                             unsigned long flags);
     */ 
    // Create the perf_event for this thread on all CPUs with no event group
    int perf_fd = perf_event_open(&pe, 0, -1, group, 0);
    if(perf_fd == -1) {
      PRINT("Failed to open perf event file: %s\n", strerror(errno));
      abort();
    }
		PRINT("Install watchpoint. perf_fd %d group %d\n", perf_fd, group); 
    
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

		//PRINT("In the end of install_watchpoint\n");   
 
    return perf_fd;
  }
  
  // Enable a setof watch points now
  void enable_watchpoints(int fd) {
    // Start the event
    if(ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
      fprintf(stderr, "Failed to enable perf event: %s\n", strerror(errno));
      abort();
    }
  }
  
  void disable_watchpoint(int fd) {
    // Start the event
    if(ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
      fprintf(stderr, "Failed to disable perf event: %s\n", strerror(errno));
      abort();
    }
  }

  // How many watchpoints that we should care about.
  int getWatchpointsNumber() {
    return _numWatchpoints;
  }
	
	bool checkAndSaveCallsite(faultyObject * object, int depth, void ** callsite) {
		return object->faultySite.saveAndCheck(depth, callsite); 
	}

  // Handle those traps on watchpoints now.  
  static void trapHandler(int sig, siginfo_t* siginfo, void* context);
 
private:
  int    _numWatchpoints;

  // Watchpoint array, we can only support 4 watchpoints totally.
  faultyObject _wp[xdefines::MAX_WATCHPOINTS]; 
};

#endif
