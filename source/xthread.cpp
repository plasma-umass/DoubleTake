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

int getThreadIndex() {
  if(!global_isInitPhase()) {
    return current->index;
  } else { 
    return 0;
  }
}

char * getThreadBuffer(void) {
  return current->outputBuf;
}

void xthread::invokeCommit() {
  xrun::getInstance().epochEnd(false);
  xrun::getInstance().epochBegin();
}
 
void xthread::epochBegin() {
  current->qlist.restore();
  current->syncevents.cleanup();
}
 
void xthread::prepareRollback() {
  syscalls::getInstance().prepareRollback();

  // Initialize the semaphores for threads at first since preparing synchronization may
  // increment corresponding semaphore.
  threadmap::getInstance().prepareRollback();

  // Update the semaphores and synchronization entry 
  _sync.prepareRollback();
}


void xthread::setThreadSafe() {
    bool toRollback = false;
    lock_thread(current);
    current->isSafe = true;

    if(current->waitSafe) {
      signal_thread(current);
    }
    unlock_thread(current);
}

bool xthread::addQuarantineList(void * ptr, size_t sz) {
  return current->qlist.addFreeObject(ptr, sz);
}

void xthread::rollback() {
  current->qlist.restore();

  // Recover the context for current thread.
  restoreContext(); 
}
