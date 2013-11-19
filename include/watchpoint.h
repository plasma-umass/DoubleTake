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

class watchpoint {
  watchpoint() {
    _numWatchpoints = 0;
  }

  ~watchpoint() {
  }

public:

  class watching {
    public:
      size_t * addr;
      size_t   faultvalue;
      bool     hasOverflowed;
  };
      
  static watchpoint& getInstance (void) {
    static char buf[sizeof(watchpoint)];
    static watchpoint * theOneTrueObject = new (buf) watchpoint();
    return *theOneTrueObject;
  }

  void initialize(void) {
    init_debug_registers();
  }

  // Add a watch point with its value to watchpoint list.
  void addWatchpoint(void * addr, size_t value ) {

//    DEBUG("***********DoubleTake: problematic address %p with value 0x%lx!!!!!\n", addr, value);
    if(_numWatchpoints < xdefines::MAX_WATCHPOINTS) {
      // Watch
      _wp[_numWatchpoints].addr = (size_t *)addr;
      _wp[_numWatchpoints].faultvalue = value;
      _wp[_numWatchpoints].hasOverflowed = false;
      _numWatchpoints++;
  //    DEBUG("***********CAUGHT: problematic address %p with value 0x%lx _numWatchpoints %d!!!!!\n", addr, value, _numWatchpoints);
    } 
  }

  bool hasToRollback(void) {
    return (_numWatchpoints > 0 ? true : false);
  }

  // Set all watch points before rollback. 
  void installWatchpoints(void)
  {
    pid_t child;
    struct sigaction trap_action;
    int status = 0;
    pid_t parent = getpid();

    DEBUG("installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
    // Initialize those debug registers. 
    init_debug_registers();

    // Now we are setting a trap handler.
    Real::sigaction()(SIGTRAP, NULL, &trap_action);
    trap_action.sa_sigaction = watchpoint::trapHandler;
    trap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
    Real::sigaction()(SIGTRAP, &trap_action, NULL);

    // Creating a child to setup the watchpoints for the parent.
    child = Real::fork()();
    if (child == 0)
    {
      sleep(1); // This is not necessarily enough but let's try it.
      // Child process.

//      DEBUG("CHILD installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    

      // Now the child will setup the debug register for its parent.
      if(ptrace(PTRACE_ATTACH, parent, NULL, NULL))
      {
	perror("DoubleTake error:");
        DEBUG("Child cannot trace the parent %d\n", parent);
        exit(-1);
      }
      //DEBUG("child install watchpoints now, before sleep\n");

      // Child will wait the parent to stop.
      sleep(1);

     // DEBUG("child install watchpoints now\n");

      // Install all watchpoints now.
      DEBUG("child install %d watchpoints at %p\n", _numWatchpoints, &_numWatchpoints);
      for(int i = 0; i < _numWatchpoints; i++) {
        DEBUG("child install watchpoints %d at %p\n", i, _wp[i].addr);
        insert_watchpoint ((unsigned long)_wp[i].addr, sizeof(void *), hw_write, parent);
      } 
      
      // Now we will deteach the parent.      
      if(ptrace(PTRACE_DETACH, parent, NULL, NULL))
      {
        DEBUG("Child can not detach the parent %d\n", parent);
        exit(-1);
      }
      exit(0);
    }
    else if(child > 0) {
      DEBUG("PARENT installWatchpoints %d watchpoints %d!!!!!!!!!\n", __LINE__, _numWatchpoints);    
      // Wait for the children to setup
      waitpid(child, &status, 0);
      if(WEXITSTATUS(status))
      {
        DEBUG("child exit now!0\n");
        exit(-1);
      }
    }
    else {
      DEBUG("Can't fork when installing watch points, with error: %s\n", strerror(errno));
      while(1);
    }
  }

  // How many watchpoints that we should care about.
  int getWatchpointsNumber(void) {
    return _numWatchpoints;
  }

  
  bool printWatchpoints(void) {
    bool isOverflowing = false;
    bool hasOverflowed = false;

    for(int i = 0; i < _numWatchpoints; i++) {
      size_t value = *_wp[i].addr;
      isOverflowing = false;
      
      // Check whether now overflow actually happens
      if(value == _wp[i].faultvalue && _wp[i].hasOverflowed == false) {
        _wp[i].hasOverflowed = true;
        isOverflowing = true;
        hasOverflowed = true;
      }

      if(!isOverflowing) {
        DEBUG("\tNow watchpoint %d: at %p value %lx Faulted Value %lx\n", i, _wp[i].addr, value, _wp[i].faultvalue);
      }
      else {
        DEBUG("\tOVERFLOW NOW!!!!Watchpoint %d: at %p value %lx Faulted Value %lx\n", i, _wp[i].addr, value, _wp[i].faultvalue);
      }
    } 

    return hasOverflowed;
  }

  // Handle those traps on watchpoints now.  
  static void trapHandler(int sig, siginfo_t* siginfo, void* context)
  {
    ucontext_t * trapcontext = (ucontext_t *)context;
    size_t * addr = (size_t *)trapcontext->uc_mcontext.gregs[REG_RIP]; // address of access
    bool isOverflow = false;

    // check the address of trap
    // Get correponding ip information.
    //DEBUG("CAPTURING write at %p: %lx at ip %lx. sivalue %d address %p, signal code %d\n", addr, *((unsigned long *)addr), trapcontext->uc_mcontext.gregs [REG_RIP], siginfo->si_value, siginfo->si_ptr, siginfo->si_code);
    // We omit those modifications by stopgap library itself.
    if(selfmap::getInstance().isStopgapLibrary(addr)) {
      return;
    }
    
    DEBUG("\n\n\nCAPTURING writes at watchpoints from ip %p, detailed information:\n", addr);
    isOverflow = watchpoint::getInstance().printWatchpoints();
    isOverflow = true;
    selfmap::getInstance().printCallStack(trapcontext, addr, isOverflow);
#ifdef STOP_AT_OVERFLOW
    if(isOverflow) {
      while(1) ;
    }
#endif
  }
 
private:
  int    _numWatchpoints;
  pid_t  _mypid; //temporary

  watching _wp[xdefines::MAX_WATCHPOINTS]; 
  pid_t  _watchedProcess;
  bool   _gotFirstTrap;
};

#endif
