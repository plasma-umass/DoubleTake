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


void xthread::setThreadSafe(void) {
    bool toRollback = false;
    WRAP(pthread_mutex_lock)(&current->mutex);
    current->isSafe = true;
    WRAP(pthread_mutex_unlock)(&current->mutex);

    global_lock();
    // Now check the state of global, if now it is at epoch end, 
    if(globalinfo::getInstance().isEpochEnd()) {
      global_unlock();

      // Wait on global mutex until all threads are waiting there;
      globalinfo::getInstance().waitForNotification();

      // check whether we should rollback?    
      global_lock();
      toRollback = globalinfo::getInstance().isRollback();
      global_unlock();
 
      if(toRollback) {
        WRAP(pthread_mutex_lock)(&current->mutex);

        // Waiting for the waking up from the committing thread
        while(current->status != E_THREAD_ROLLBACK) {
          WRAP(pthread_cond_wait)(&current->cond, &current->mutex);
        }
      
        WRAP(pthread_mutex_unlock)(&current->mutex);
      }
      // When we do not need to rollback, move forward now. 
    }
    else {
      global_unlock();
    }
}
