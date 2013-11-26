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
 * @file   watchpoint.cpp
 * @brief  Including the implemenation of watch point handler.
 *         Since we have to call functions in other header files, then it is 
 *         impossible to put this function into the header file.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "watchpoint.h"

// Handle those traps on watchpoints now.  
void watchpoint::trapHandler(int sig, siginfo_t* siginfo, void* context)
{
  ucontext_t * trapcontext = (ucontext_t *)context;
  size_t * addr = (size_t *)trapcontext->uc_mcontext.gregs[REG_RIP]; // address of access

  fprintf(stderr, "\n\n\nCAPTURING writes at watchpoints from ip %p, detailed information:\n", addr);
  isOverflow = watchpoint::getInstance().printWatchpoints();
  isOverflow = true;
  selfmap::getInstance().printCallStack(trapcontext, addr, isOverflow);
}
 
