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
 * @file   xsync.h
 * @brief  Mapping between pthread_t and internal thread information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _SYNCMAP_H_
#define _SYNCMAP_H_

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

class xsync {

public:
  xsync()
  {
  }
  
  void initialize() {
    _smap.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::SYNCMAP_SIZE);

    // initialize the global event list.
    listInit(&_glist.list);
    _glist.syncVariable = NULL;
    _glist.syncCmd = E_SYNC_SPAWN;

    WRAP(pthread_mutex_init)(&_glist.lock, NULL);
  } 

  void insertSyncMap(void * key, struct syncEventList * list) {
    _smap.insert(key, sizeof(key), list);
  }

  // Allocate a new synchronization event list
  struct syncEventList * allocateSyncEventList(void * var, thrSyncCmd synccmd) {
    struct syncEventList * list = NULL;
    
    // Create an synchronziation event.
    list = (struct syncEventList *)InternalHeap::getInstance().malloc(sizeof(*list));

    // Initialize the sequence number   
    list->syncVariable = var;
    list->syncCmd = synccmd;
    listInit(&list->list);

    WRAP(pthread_mutex_init)(&list->lock, NULL);

    // Add this event list into the map.
    insertSyncMap(var, list);
    return list;
  }

  // Record a synchronization event
  void recordSyncEvent(void * var, thrSyncCmd synccmd) {
    struct syncEventList * list = NULL;

    if(synccmd == E_SYNC_SPAWN) {
      list = &_glist;
    }
    else {
      assert(var != NULL);

      if(!_smap.find(var, sizeof(void *), &list)) {
        //PRDBG("need to allocation a syncEventList\n");
        list = allocateSyncEventList(var, synccmd);
      }
    }

    assert(list != NULL);
    struct syncEvent * event;
    event = (struct syncEvent *)InternalHeap::getInstance().malloc(sizeof(struct syncEvent));
    
    if(event == NULL) {
      PRDBG("InsertSyncEventList failed because we do not have enough memory?\n");
      WRAP(exit)(-1);
    }
  
    listInit(&event->list);
    listInit(&event->threadlist);

    // Change the event there.
    event->thread = current;
    event->eventlist = list;
    PRDBG("RECORDING: syncCmd %d on variable %p event %p thread %p (THREAD%d)", synccmd, var, event, event->thread, current->index);

    if(synccmd != E_SYNC_LOCK) {    
      WRAP(pthread_mutex_lock)(&list->lock);
      listInsertTail(&event->list, &list->list);
      WRAP(pthread_mutex_unlock)(&list->lock);
    }
    else {
      // We only record synchronization inside critical section.
      // so there is no need to acquire another lock.
      listInsertTail(&event->list, &list->list);
    }

    // Insert to thread sync list. No need to lock since only this thread
    // will insert events to this list
    listInsertTail(&event->threadlist, &current->syncevents.list);
  }


  /*
   Unpdate synchronization event in the rollback phase.
   Normally, this function will update two list: 
     The synchronization variable's correponding event list.
     The thread's event list.
  */
  void updateSyncEvent(void * var, thrSyncCmd synccmd) {
    struct syncEventList * eventlist = NULL;

    global_lock();

    if(synccmd == E_SYNC_SPAWN) {
      eventlist = &_glist;
    }
    else {
      assert(var != NULL);

      if(!_smap.find(var, sizeof(void *), &eventlist)) {
        assert(eventlist != NULL);
      }
    }
    
    PRWRN("UPDATING before: eventlist current event %p, thread event %p. variable %p eventlist variable %p\n", eventlist->curentry, getThreadEvent(current->syncevents.curentry), var, eventlist->syncVariable);
    assert(eventlist->curentry == (list_t *)getThreadEvent(current->syncevents.curentry));

    if(eventlist->syncVariable != var) {
      fprintf(stderr, "assert, eventlist syncVariable %p var %p\n", eventlist->syncVariable, var);
      assert(0);
    }
   //  struct syncEvent * curEvent = (struct syncEvent *)eventlist->curentry;

    // Whether next synchronization in this eventlist is still on the same thread?
    struct syncEvent * nextEventOfList = NULL;
    struct syncEvent * nextEventOfThread = NULL;
    list_t * nextEntry = NULL;
    //nextEvent = updateSyncEventList(eventlist);
    // Update next event of eventlist.
    nextEventOfList = (struct syncEvent *)updateNextEvent(eventlist);
    
    // Update next event of thread eventlist.
    nextEntry = updateNextEvent(&current->syncevents);
    if(nextEntry != NULL) {
      nextEventOfThread = getThreadEvent(nextEntry);
    }

    // if next event of list existing, signal next thread.
    if(nextEventOfList) {
      PRLOG("calling signalNextThread");
      //PRLOG("calling signalNextThread, nextEventOfList %p nextEventofThread %p", nextEventOfList, nextEventOfThread);
      // Signal next thread on the event list 
      signalNextThread(nextEventOfList);
    }
   
    // When nextEventOfThread is equal to nextEventOfList, there is no need 
    // to signal since signalNextThread should already signal myself
    if(nextEventOfThread && (nextEventOfThread != nextEventOfList)) {   
      PRLOG("calling signalCurrentThread");
      // Send semaphore to myself if some pending events becomes the top of my list.
      signalCurrentThread(nextEventOfThread);
    }
    
    PRWRN("UPDATING after: eventlist next event %p, next thread event %p\n", nextEventOfList, nextEventOfThread);

//    PRWRN("Updating: eventlist current event %p, thread event %p before updating syncevents\n", eventlist->curentry, getThreadEvent(current->syncevents.curentry));
    global_unlock();
  }
 
  // Signal next thread on the same synchronization variable.
  void signalNextThread(struct syncEvent * event) {
    thread_t * thread = NULL;

    thread = event->thread;

    // We should check whether this event is on the top of thread.
    if(isThreadNextEvent(event, thread)) {
      signalThread(thread);
      PRWRN("THREAD%d actually signal next thread %d on event %p", current->index, thread->index, event);
    }
    else {
      PRWRN("THREAD%d adding pending event to next thread %d on event %p", current->index, thread->index, event);
      addPendingSyncEvent(event, thread);
    }
  }
  
  // Signal current thread if next event is the top on its list.
  void signalCurrentThread(struct syncEvent * event) {
    thread_t * thread = event->thread;
    struct syncEventList * eventlist = &thread->pendingSyncevents;
    
    assert(thread == current);

    PRWRN("singalCurrentThread: event %p on variable %p command %d", event, event->eventlist->syncVariable, event->eventlist->syncCmd); 
    //PRWRN("singalCurrentThread: event %p on variable %p command %d thread %p, current %p\n", event, event->eventlist->syncVariable, event->eventlist->syncCmd, thread, current); 

    if(!isListEmpty(&eventlist->list)) {
 //     PRWRN("singalCurrentThread: event %p thread %p, pending list is not empty!!!\n", event, thread); 
      // Only signal itself when current event is first event of this thread.
      struct pendingSyncEvent * pe = NULL; 

      // Search the whole list for given tid.
      pe = (struct pendingSyncEvent *)nextEntry(&eventlist->list);
 //     PRLOG("singalCurrentThread: event %p thread %p, pending list is not empty, pe->event %p!!!\n", event, thread, pe->event); 
      while(true) {
        // We found this event
        if(pe->event == event) {
          PRLOG("singalCurrentThread: signal myself, retrieve event %p pe->event %p", event, pe->event); 
          // Remove this event from the list.
          listRemoveNode(&pe->list);
          
          // Release corresponding memory to avoid memory leakage.
          InternalHeap::getInstance().free(pe);

          // Now signal current thread.
          signalThread(thread);
          break;
        }
 
        // Update to the next thread.
        if(isListTail(&pe->list, &eventlist->list)) {
          break;
        }
        else {
          pe = (struct pendingSyncEvent *)nextEntry(&pe->list);
        } 
      } // while (true)
    } 
    else {
      PRLOG("thread pending list is empty now!!!");
    }
  }
 
  // Update the current event entry in an event lsit.
  list_t * updateNextEvent(struct syncEventList * list) {
    list_t * curentry = list->curentry;

    if(!isListTail(curentry, &list->list)) {
    // (stderr, "Not tail, UpdateNextEvent curentry %p, list->list %p", curentry, &list->list);
      list->curentry = nextEntry(curentry);
    }
    else {
     // PRDBG("tail, UpdateNextEvent curentry %p, list->list %p", curentry, &list->list);
      list->curentry = NULL;
    }

    return list->curentry;
  }
 
  // Get event from the thread event list 
  inline void updateMutexSyncList(pthread_mutex_t * mutex) {
    struct syncEventList * eventlist = NULL;

    global_lock();

    assert(mutex != NULL);

    if(!_smap.find((void *)mutex, sizeof(void *), &eventlist)) {
      assert(eventlist != NULL);
    }
    
    if(eventlist->syncVariable != (void *)mutex) {
      fprintf(stderr, "assert, eventlist syncVariable %p var %p\n", eventlist->syncVariable, mutex);
      assert(0);
    }

    // Whether next synchronization in this eventlist is still on the same thread?
    struct syncEvent * nextEvent = NULL;
    // Update next event of eventlist.
    nextEvent = (struct syncEvent *)updateNextEvent(eventlist);
   
    // if next event of list existing, signal next thread.
    if(nextEvent) {
      PRLOG("calling signalNextThread");
      // Signal next thread on the event list 
      signalNextThread(nextEvent);
      // FIXME
      // Since we haven't look at 
      //FIXME
      //we may already later than thread event? How to fix this?
    }
   
//    PRWRN("Updating: eventlist current event %p, thread event %p before updating syncevents\n", eventlist->curentry, getThreadEvent(current->syncevents.curentry));
    global_unlock();
  }

  void updateThreadSyncList(pthread_mutex_t *  mutex) {
    list_t * nextEntry = NULL;
    
    global_lock();
    // Update next event of thread eventlist.
    nextEntry = updateNextEvent(&current->syncevents);
    if(nextEntry != NULL) {
      struct syncEvent * nextEventOfThread = NULL;
      nextEventOfThread = getThreadEvent(nextEntry);

      signalCurrentThread(nextEventOfThread); 
    }
    global_unlock();
  }

  // How to return a thread event from specified entry.
  inline struct syncEvent * getThreadEvent(list_t * entry) {
    // FIXME: if we changed the structure of syncEvent.
    static int threadEventOffset = sizeof(list_t);
    
    return(struct syncEvent *)((intptr_t)entry - threadEventOffset);
  }

  // Update the current event entry in an event lsit.
  struct syncEvent * updateSyncEventList(struct syncEventList * list) {
    bool hasUpdated = false;
    struct syncEvent * event = NULL;
    event = (struct syncEvent *)updateNextEvent(list);
    if(event) {
      // Signal next thread  
      signalNextThread(event);
    }

    return event;
  }

  void cleanupSynchronizations(void) {
    // we are going to make sure that there is no problem caused by synchronzation.

    // We will try to put semaphores for those threads acquiring the same synchronization.

    // Also, we might try to increment the semaphore for current thread too.
  }

  /*
   How to reproduce the synchronization - using the semaphore to do this:
     a. One thread only have one semaphore but multiple synchronization variable. And we donot have
     one global order for all synchronization because we want to avoid the performance 
     bottleneck caused by the global order in the recording phase.
     But we want to avoid wrong order of synchronization.
     For example, in a system with only 3 threads.
     T1: lock1(S1)                       lock1(S5),
     T2:       lock1(S2), lock2(S3),
     T3:                           lock2(S4)
     T4: lock3
     lock1: S1, S2, S5
     lock2: S3, S4
     
     Initially, only T1 can acquire the semaphore that is because of the following rule. We are trying
     to check all synchronization variables one by one. Initially, we only increment the semaphore 
     of a thread when the following two conditions meet.
        a. The thread is the first thread on one synchronization variable.
        b. This synchronization is the first synchronization for this thread.
     For the above case, T2's semaphore and T3 semaphore won't be incremented, but T1 and T4 should. 
     For T2, lock2 is not the first synchronization on T2. 
     For T3, T3 is not the first thread on lock2.
 
     Second step for T1: 
     We can find out the T2(S2) will acquire lock1, so it increment the semaphore for T2.

     Second step for T2:
     After T2 acquire the semaphore, it will check two things:
     a. What's the next thread to work on current synchronization variable. 
        it will try to increment the semaphore for T1.
     b. What's next synchronization of current thread and whether it is the head of this variable. 
        For example, S2 is next statement and it is located
     in the head of list. Then it also increment the semaphore of T2 so that T2 can acquire lock2.
   */

  struct syncEventList * getSyncEventList(void * var) {
    struct syncEventList * list = NULL;
    _smap.find(var, sizeof(void *), &list);
    return list;
  } 
 
  void deleteMap(void * key) {
    _smap.erase(key, sizeof(void*));
  }

  // cleanup all events in a list.
  void cleanEventList(struct syncEventList * eventlist) {
    list_t * head = &eventlist->list;
    list_t * entry = NULL;

    while((entry = listRetrieveItem(head)) != NULL) {
      InternalHeap::getInstance().free(entry);
    }

    listInit(head);
  }

  void cleanSyncEvents() {
    // Remove all events in the global map and event list.
    syncHashMap::iterator i;
    struct syncEventList * eventlist;

    for(i = _smap.begin(); i != _smap.end(); i++) {
      eventlist = (struct syncEventList *)i.getData();

      cleanEventList(eventlist);
    }

    cleanEventList(&_glist);
  }

  /*
   Prepare rollback. Only one thread can call this function.
   It basically check every synchronization variable.
   If a synchronization variable is in the head of a thread, then
   we try to UP corresponding thread's semaphore.
   */
  void prepareRollback() {
    syncHashMap::iterator i;
    struct syncEventList * eventlist;
    struct syncEvent * event;
    thread_t * thread;

    for(i = _smap.begin(); i != _smap.end(); i++) {
      eventlist = (struct syncEventList *)i.getData();
      
      prepareEventListRollback(eventlist);
    }

    prepareEventListRollback(&_glist);
  }

  /*
    Prepare the rollback for an event list.
    It will check whether the first event in the event list is also the head of specific 
    thread. If yes, then we will signal specific thread so that this thread can acquire
    the semaphore immediately.
   */
  void prepareEventListRollback(struct syncEventList * eventlist) {
    struct syncEvent * event;
    thread_t * thread = NULL;

    if(!isListEmpty(&eventlist->list)) {
      event = (struct syncEvent *)nextEntry(&eventlist->list); 
      thread = event->thread;
  
      // Make current entry pointing to the first entry    
      eventlist->curentry = (list_t *)event;
      
      // Check whether this event is first event of corresponding thread
      if(isThreadNextEvent(event, thread)) {
        signalThread(thread);
      }
      else {
        PRLOG("Synchronization varible %p commdn %d: Add pending synchronization event %p to THREAD %d\n", eventlist->syncVariable, eventlist->syncCmd, event, thread->index);
        addPendingSyncEvent(event, thread);
      }
    }
  }

  // Add one synchronization event into the pending list of a thread.  
  void addPendingSyncEvent(struct syncEvent * event, thread_t * thread) {
    struct pendingSyncEvent * pendingEvent = NULL;

    pendingEvent = (struct pendingSyncEvent *)InternalHeap::getInstance().malloc(sizeof(struct pendingSyncEvent));

    listInit(&pendingEvent->list);
    pendingEvent->event = event;

    // Add this pending event into corresponding thread.
    listInsertTail(&pendingEvent->list, &thread->pendingSyncevents.list);
  }

  // Check whether this event is the first event of corresponding thread.
  bool isThreadNextEvent(struct syncEvent * event, thread_t * thread) {
    struct syncEventList * eventlist = &thread->syncevents;
    
    return (&event->threadlist == eventlist->curentry);
  }
  
/* 
  bool isListNextEvent(struct syncEvent * event, struct syncEventList * eventlist) {
    return (prevEntry(&event->threadlist) == eventlist->curentry);
  }
*/

  void signalThread(thread_t * thread) {
    semaphore * sema = &thread->sema;
    PRDBG("Signal semaphore %p to thread%d (at %p)\n", sema, thread->index, thread);
    sema->put();
  }

private:
  void global_lock() {
    globalinfo::getInstance().lock();
  }
  
  void global_unlock() {
    globalinfo::getInstance().unlock();
  }

  size_t getThreadSyncSeqNum() {
    return 0;
  }

  semaphore * getThreadSemaphore(thread_t * thread) {
    return &thread->sema;
  }

#if 0
  // Get the first thread on this synchronization event list.
  thread_t * getFirstThread(struct syncEventList * eventlist) {
    struct syncEvent * event;
    thread_t * firstthread = NULL; 
    if(!isListEmpty(&eventlist->list)) {
      event = (struct syncEvent *)nextEntry(&eventlist->list); 

      firstthread = event->thread;
    }

    return firstthread;
  }
#endif

  // We are maintainning a private hash map for each thread.
  typedef HashMap<void *, struct syncEventList *, spinlock, InternalHeapAllocator> syncHashMap;

  // Synchronization related to different sync variable should be recorded into
  // the synchronization variable related list.
  syncHashMap _smap;

  // Thread spawning should go into this global list.
  struct syncEventList _glist;
};

#endif
