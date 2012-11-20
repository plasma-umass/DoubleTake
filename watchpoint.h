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
 * @brief  Watch point handler.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *         Adopt the code from http://stackoverflow.com/questions/8941711/is-is-possible-to-set-a-gdb-watchpoint-programatically, the original code is written by .
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
#include <linux/user.h>
#include <ucontext.h>
#include "xdefines.h"

extern "C" {
  typedef struct {
    char l0:1;
    char g0:1;
    char l1:1;
    char g1:1;
    char l2:1;
    char g2:1;
    char l3:1;
    char g3:1;
    char le:1;
    char ge:1;
    char pad1:3;
    char gd:1;
    char pad2:2;
    char rw0:2;
    char len0:2;
    char rw1:2;
    char len1:2;
    char rw2:2;
    char len2:2;
    char rw3:2;
    char len3:2;
  } dr7_t;
};

class watchpoint {
  enum {
    DR7_BREAK_ON_EXEC  = 0,
    DR7_BREAK_ON_WRITE = 1,
    DR7_BREAK_ON_RW    = 3,
  };

  enum {
    DR7_LEN_1 = 0,
    DR7_LEN_2 = 1,
    DR7_LEN_4 = 3,
  };

  watchpoint() {
    _numWatchpoints = 0;
  }

  ~watchpoint() {

  }

public:
  // The singleton
  static watchpoint& getInstance (void) {
    static char buf[sizeof(watchpoint)];
    static watchpoint * theOneTrueObject = new (buf) watchpoint();
    return *theOneTrueObject;
  }


  // Add a watch point to watchpoint list.
  void addWatchpoint(void * addr) {
    if(_numWatchpoints < xdefines::MAX_WATCHPOINTS) {
      // Watch
      _watchpoints[_numWatchpoints] = addr;
      _numWatchpoints++;
    } 
  }

  // Set a watch point for an specified address
  int installWatchpoints(void)
  {
    pid_t child;
    pid_t parent = getpid();
    struct sigaction trap_action;
    int child_stat = 0;

    // Now we are setting a trap handler.
    sigaction(SIGTRAP, NULL, &trap_action);
    trap_action.sa_sigaction = watchpoint::trapHandler;
    trap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;
    sigaction(SIGTRAP, &trap_action, NULL);

    // Creating a child, then setup the watchpoint.
    // Then child can exit now.
    if((child = fork()) == 0)
    {
        // Now the child will ptrace its parent.
        int retval = EXIT_SUCCESS;

        dr7_t dr7 = {0};
        dr7.l0 = 1;
        //dr7.rw0 = DR7_BREAK_ON_RW;
        dr7.rw0 = DR7_BREAK_ON_WRITE;
        dr7.len0 = DR7_LEN_4;

        if (ptrace(PTRACE_ATTACH, parent, NULL, NULL))
        {
            exit(EXIT_FAILURE);
        }

        sleep(1);
        fprintf(stderr, "child after usleep\n");

        int i;
        for(int i = 0; i < _numWatchpoints; i++) {
          fprintf(stderr, "Installing watchpoint %d: %p\n", i, _watchpoints[i]);
          if (ptrace(PTRACE_POKEUSER, parent, offsetof(struct user, u_debugreg[0]), _watchpoints[i]))
          {
            retval = EXIT_FAILURE;
          }

          if (ptrace(PTRACE_POKEUSER, parent, offsetof(struct user, u_debugreg[7]), dr7))
          {
            retval = EXIT_FAILURE;
          }
        }

        if (ptrace(PTRACE_DETACH, parent, NULL, NULL))
        {
            retval = EXIT_FAILURE;
        }

        fprintf(stderr, "child exit\n");
        exit(retval);
    }

    // Wait for the children to setup
    waitpid(child, &child_stat, 0);
    if (WEXITSTATUS(child_stat))
    {
        fprintf(stderr, "child exit now!0\n");
        return 1;
    }

    return 0;
  }

  static void trapHandler(int sig, siginfo_t* siginfo, void* context)
  {
    ucontext_t * trapcontext = (ucontext_t *)context;
     void * addr = siginfo->si_addr; // address of access

    // check the address of trap
    // Get correponding ip information.
#ifdef X86_32BIT    
    fprintf(stderr, "CAPTURING write at %p: %lx at ip %lx\n", addr, *((unsigned long *)addr), trapcontext->uc_mcontext.gregs [REG_EIP]);
#else
    fprintf(stderr, "CAPTURING write at %p: %lx at ip %lx\n", addr, *((unsigned long *)addr), trapcontext->uc_mcontext.gregs [REG_RIP]);
#endif
  }

private:
  int    _numWatchpoints; 
  void * _watchpoints[xdefines::MAX_WATCHPOINTS]; 
};

#endif
