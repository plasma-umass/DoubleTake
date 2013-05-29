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
 * @file   xthread.cpp
 * @brief  Handle Thread related information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xdefines.h"
#include "xthread.h"
#include "xrun.h"
#include "syscalls.h"
#include "threadmap.h"


// Global lock used when joining and exiting a thread.
//threadmap::threadHashMap threadmap::_xmap;
//list_t threadmap::_alivethreads;

//bool xthread::_isRollbackPhase;
//list_t xthread::_deadSyncVars;
__thread thread_t * current;
threadmap::threadHashMap threadmap::_xmap;
list_t threadmap::_alivethreads;

int getThreadIndex(void) {
  if(!globalinfo::getInstance().isInitPhase()) {
  //  PRWRN("current %p\n", current);
    return current->index;
  }
  else { 
    return 0;
  }
}

char * getThreadBuffer(void) {
  int index = getThreadIndex();

  return threadinfo::getInstance().getThreadBuffer(index);
}

void xthread::invokeCommit(void) {
  xrun::getInstance().epochEnd();
  xrun::getInstance().epochBegin();
}
  
void xthread::prepareRollback(void) {
    syscalls::getInstance().prepareRollback();

    // We only prepare the rollback for multithreading program.
    if(!globalinfo::getInstance().isMultithreading()) {
      return;
    }

    // Try to create the semaphores for all threads and 
    // backup all thread context.
    _thread.prepareRollback();
}
