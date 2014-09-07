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

    //PRINT("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
		// We don't need to setup watchpoints if it is 0.
		if(_numWatchpoints == 0) {
			return;
		}

    // Initialize those debug registers. 
    init_debug_registers();

    // Now we are setting a trap handler for myself.
    trap_action.sa_sigaction = watchpoint::trapHandler;
    trap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
    Real::sigaction(SIGTRAP, &trap_action, NULL);

		// Setup the watchpoints information and notify the daemon (my parent)
		struct watchpointsInfo wpinfo;
		wpinfo.count = _numWatchpoints;
		for(int i = 0; i < _numWatchpoints; i++) {
			PRINT("Watchpoint %d: addr %p. _numWatchpoints %d\n", i, _wp[i].faultyaddr, _numWatchpoints);
			wpinfo.wp[i] = (unsigned long)_wp[i].faultyaddr;
			PRINT("Watchpoint %d: addr %p after setup\n", i, _wp[i].faultyaddr);
			// Update the values of those faulty address so that 
			// we can compare those values to find out which watchpoint 
			// are accessed since we don't want to check the debug status register
			_wp[i].currentvalue = *((unsigned long *)_wp[i].faultyaddr);
		}	 

		// Notify the parent about those watchpoints information. 
		Real::write(writePipe(), &wpinfo, sizeof(struct watchpointsInfo));

		sleep(3);
/*
		// Now we are waiting on the readPipe in order to proceed.
		int ret;
		int readSize;
		if(Real::read(readPipe(), &ret, sizeof(ret)) == 0) {
			// Something must be wrong.
			PRERR("Reading from the pipe failed, with error %s\n", strerror(errno));
			assert(0);
		}
		PRINT("Watchpoint setup done! Totalsize %d. count %ld\n", sizeof(wpinfo), wpinfo.count);

		PRINT("Now we are going to rollback\n");
*/
		// We actually don't care about what content.
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
