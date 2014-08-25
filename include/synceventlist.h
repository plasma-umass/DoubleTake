// -*- C++ -*-

/*
 Copyright (c) 2007-2013 , University of Massachusetts Amherst.

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
 * @file   synceventlist.h
 * @brief  Manage the list of synchronization event. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _SYNCEVENTLIST_H_
#define _SYNCEVENTLIST_H_

#include <sys/types.h>
#include <syscall.h>
#include <sys/syscall.h>
#include "xdefines.h"
#include "hashmap.h"
#include "hashfuncs.h"
#include "threadmap.h"
#include "spinlock.h"
#include "semaphore.h"
#include "globalinfo.h"
#include "threadstruct.h"

typedef enum e_thrsynccmd {
  E_SYNC_SPAWN = 0,  // Thread creation
//  E_SYNC_COND_SIGNAL,// conditional signal
//  E_SYNC_COND_BROADCAST,// conditional broadcast
//  E_SYNC_COND_WAIT,// conditional wait
//  E_SYNC_COND_WAKEUP,// conditional wakeup from waiting
  E_SYNC_BARRIER, // barrier waiting 
  E_SYNC_MUTEX_LOCK,
  E_SYNC_MUTEX_TRY_LOCK,
  E_SYNC_RWLOCK_RDLOCK,
  E_SYNC_RWLOCK_TIMEDRDLOCK,
  E_SYNC_RWLOCK_TRYRDLOCK,
  E_SYNC_RWLOCK_WRLOCK,
  E_SYNC_RWLOCK_TIMEDWRLOCK,
  E_SYNC_RWLOCK_TRYWRLOCK,
  E_SYNC_THREAD, // Not a actual synchronization event, but for each thread.
//  E_SYNC_KILL, // Inside the critical section. 
} thrSyncCmd;

class SyncEventList {

public:
  SyncEventList(void * variable, thrSyncCmd synccmd)
  {
//    PRINF("synceventlist initialization at list %p\n", &list);
    // Initialize the sequence number   
    listInit(&list);
    Real::pthread_mutex_init(&lock, NULL);
    syncVariable = variable;
    syncCmd = synccmd;
    curentry = NULL;
  }
  
  // Record a synchronization event
  void recordSyncEvent(thrSyncCmd synccmd, int ret) {
    struct syncEvent * event = allocSyncEvent();

    PRINF("recordSyncEvent event %p thread %p eventlist %p\n", event, current, this);
//    PRINT("recordSyncEvent line %d: event %p thread %p eventlist %p\n", __LINE__, event, current, this);
    listInit(&event->list);

    // Change the event there.
    event->thread = current;
    event->eventlist = this;
    event->ret = ret;

  //  PRINT("recordSyncEvent line %d: event %p thread %p eventlist %p\n", __LINE__, event, current, this);
    if(synccmd != E_SYNC_MUTEX_LOCK) {   
      Real::pthread_mutex_lock(&this->lock);
      listInsertTail(&event->list, &this->list);
      Real::pthread_mutex_unlock(&this->lock);
    }
    else {
		if((unsigned long)this == 0x100001cffe48) {
    PRINT("recordSyncEvent line %d: event %p thread %p eventlist %p nowthis %lx\n", __LINE__, event, current, this, *((unsigned long *)this));
   	PRINT("event->list %p this->list %p\n", &event->list, &this->list);
		}
	    // We only record synchronization inside critical section.
      // so there is no need to acquire another lock.
      listInsertTail(&event->list, &this->list);
		if((unsigned long)this == 0x100001cffe48) {
    	PRINT("recordSyncEvent line %d: event %p thread %p eventlist %p nowthis %lx\n", __LINE__, event, current, this, *((unsigned long *)this));
		}
    }
   // PRINF("RECORDING: syncCmd %d on event %p thread %p (THREAD%d)", synccmd, event, event->thread, current->index);
  }


  inline void * getSyncVariable() {
    return syncVariable;
  }

  inline thrSyncCmd getSyncCmd() {
    return syncCmd; 
  }
 
  // Move forward for current thread event list 
  inline struct syncEvent * advanceSyncEvent() {
    list_t * curentry = this->curentry;
		
		PRINT("currenty is %p this->list %p with address %p\n", curentry, this->list, &this->list);
    if(!isListTail(curentry, &this->list)) {
      this->curentry = nextEntry(curentry);
    }
    else {
      this->curentry = NULL;
    }

    return (struct syncEvent *)this->curentry;
  }

  
  // cleanup all events in a list.
  void cleanup() {
    listInit(&this->list);
  }

  struct syncEvent * allocSyncEvent() {
    return current->syncevents.alloc();
  }

  // Set the first entry and return it
  struct syncEvent * prepareRollback() {
    struct syncEvent * event = NULL;

    if(!isListEmpty(&list)) {
      this->curentry = nextEntry(&list);
      event = (struct syncEvent *)this->curentry;
    }

//    PRINF("synceventlist at %p event %p\n", &list, event);
    return event;
  }

private:
  list_t list; // List for all synchronization events.
  pthread_mutex_t lock;
  void     * syncVariable;
  thrSyncCmd syncCmd;
  list_t   * curentry; 
};

#endif
