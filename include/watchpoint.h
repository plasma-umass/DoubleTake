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
 * @file   watchpoint.h
 * @brief  Watch point handler, we are relying the dr.c to add watchpoint and detect the condition of 
 *         watch point, dr.c is adopted from GDB-7.5/gdb/i386-nat.c.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _WATCHPOINT_H_
#define _WATCHPOINT_H_


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
#include <ucontext.h>

#include "xdefines.h"
#include "real.h"
#include "dr.h"
#include "selfmap.h"
#include "callsite.h"

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
    init_debug_registers();
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
			//	PRINT("WARNING: we %d points, currentvalue %lx value %lx\n", trigPoints, _wp[i].currentvalue, value);
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
    pid_t child;
    struct sigaction trap_action;
    int status = 0;
    pid_t parent = getpid();

    //PRINT("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
		if(_numWatchpoints == 0) {
			return;
		}
    //PRINF("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
    // Initialize those debug registers. 
    init_debug_registers();

    // Now set up a signal handler for SIGSEGV events.
   // struct sigaction siga;
   // sigemptyset(&siga.sa_mask);

    // Set the following signals to a set
  //  sigaddset(&siga.sa_mask, SIGTRAP);
  //  sigprocmask(SIG_BLOCK, &siga.sa_mask, NULL);

    // Now we are setting a trap handler.
    trap_action.sa_sigaction = watchpoint::trapHandler;
    trap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
    Real::sigaction(SIGTRAP, &trap_action, NULL);
//    sigprocmask(SIG_UNBLOCK, &siga.sa_mask, NULL);
    PRINT("before fork %d Real::fork at %p!!!!!!!!!\n", __LINE__, Real::fork);    

    // Creating a child to setup the watchpoints for the parent.
    child = Real::fork();
    if (child == 0) {
      PRINT("child install watchpoints now, before sleep\n");
      sleep(10); // This is not necessarily enough but let's try it.

      // Now the child will setup the debug register for its parent.
      if(ptrace(PTRACE_ATTACH, parent, NULL, NULL))
      {
        PRWRN("Child cannot trace the parent %d. Error %s\n", parent, strerror(errno));
        exit(-1);
      }

      PRINT("child install watchpoints now, before sleep\n");
      // Child will wait the parent to stop.
      sleep(1);

      PRINT("child install watchpoints now\n");

      // Install all watchpoints now.
      for(int i = 0; i < _numWatchpoints; i++) {
        insert_watchpoint ((unsigned long)_wp[i].faultyaddr, sizeof(void *), hw_write, parent);
      } 
      
      // Now we will deteach the parent.      
      if(ptrace(PTRACE_DETACH, parent, NULL, NULL))
      {
        PRWRN("Child can not detach the parent %d\n", parent);
        exit(-1);
      }
      exit(0);
    }
    else if(child > 0) {
      PRINF("PARENT installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
      // Wait for the children to setup
      waitpid(child, &status, 0);
      if(WEXITSTATUS(status))
      {
        PRWRN("child exit now!0\n");
        exit(-1);
      }

			// Update all values of watchpoints before re-execution. 
      for(int i = 0; i < _numWatchpoints; i++) {
				// Updatr the current value for this faulty address so that we can compare its value later.
				_wp[i].currentvalue = *((unsigned long *)_wp[i].faultyaddr);
      } 
    }
    else {
      PRINT("Can't fork when installing watch points, with error: %s\n", strerror(errno));
      while(1);
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
