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
#include "quarantine.h"

// Global lock used when joining and exiting a thread.
//threadmap::threadHashMap threadmap::_xmap;
//list_t threadmap::_alivethreads;

//bool xthread::_isRollbackPhase;
//list_t xthread::_deadSyncVars;
__thread thread_t * current;
//threadmap::threadHashMap threadmap::_xmap;
list_t threadmap::_alivethreads;

int getThreadIndex(void) {
  if(!global_isInitPhase()) {
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
  xrun::getInstance().epochEnd(false);
  xrun::getInstance().epochBegin();
}
 
void xthread::epochBegin(void) {
  current->qlist.restore();
  current->syncevents.cleanup();
}
 
void xthread::prepareRollback(void) {
  syscalls::getInstance().prepareRollback();

  // Initialize the semaphores for threads at first since preparing synchronization may
  // increment corresponding semaphore.
  threadmap::getInstance().prepareRollback();

  // Update the semaphores and synchronization entry 
  _sync.prepareRollback();
}


void xthread::setThreadSafe(void) {
    bool toRollback = false;
    lock_thread(current);
    current->isSafe = true;

    if(current->waitSafe) {
      signal_thread(current);
    }
    unlock_thread(current);
}

void xthread::addQuarantineList(void * ptr, size_t sz) {
  current->qlist.addFreeObject(ptr, sz);
}

void xthread::rollback(void) {
  current->qlist.restore();

  // Recover the context for current thread.
  restoreContext(); 
}
