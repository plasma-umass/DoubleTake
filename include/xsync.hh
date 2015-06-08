#if !defined(DOUBLETAKE_XSYNC_H)
#define DOUBLETAKE_XSYNC_H

/*
 * @file   xsync.h
 * @brief  Mapping between pthread_t and internal thread information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <pthread.h>
#include <stddef.h>

#include "globalinfo.hh"
#include "hashfuncs.hh"
#include "hashmap.hh"
#include "internalheap.hh"
#include "list.hh"
#include "log.hh"
#include "mm.hh"
#include "recordentries.hh"
#include "semaphore.hh"
#include "spinlock.hh"
#include "synceventlist.hh"
#include "threadstruct.hh"
#include "xdefines.hh"

class xsync {
	
  struct SyncEntry {
		syncVariableType type; 
    void* realEntry;
    SyncEventList* list;
  };

public:
  xsync() {}

  void initialize() {
    _syncvars.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::SYNCMAP_SIZE);
  }

  void insertSyncMap(syncVariableType type, void* key, void* realentry, SyncEventList* list) {
    struct SyncEntry* entry =
        (struct SyncEntry*)InternalHeap::getInstance().malloc(sizeof(struct SyncEntry));
		entry->type = type;
    entry->realEntry = realentry;
    entry->list = list;
    _syncvars.insert(key, sizeof(key), entry);

		PRINF("insertSyncMap entry %p entry %p\n", realentry, entry);
  }

  void deleteMap(void* key) {
    struct SyncEntry* entry;
    if(_syncvars.find(key, sizeof(key), &entry)) {
      InternalHeap::getInstance().free(entry);
    }
    _syncvars.erase(key, sizeof(void*));
  }

  // Signal next thread on the same synchronization variable.
  void signalNextThread(struct syncEvent* event) {
    thread_t* thread = (thread_t*)event->thread;

		// Acquire the lock before adding an event to the pending list
		// since the thread may try to check whether it can proceed or not
    lock_thread(thread);

    // Add this pending event into corresponding thread.
    // Whether this event is on the top of thread?
    if(isThreadNextEvent(event, thread)) {
      // If yes, signal to this thread. There is no difference between
      // current thread or other thread.
      PRINT("Thread %d actually signal next thread %d on event %p", current->index, thread->index, event);
      signalThread(thread);
    } else {
      PRINT("Thread %d adding pending event to next thread %d on event %p", current->index,
            thread->index, event);
      addPendingSyncEvent(event, thread);
    }

		unlock_thread(thread);
  }

	// Update the synchronization list.
  void advanceThreadSyncList() {
    lock_thread(current);

    // Update next event of thread eventlist.
    current->syncevents.advanceEntry();

    unlock_thread(current);
  }


  // Signal current thread if event is one of pending events.
  void signalCurrentThread(struct syncEvent* event) {
    thread_t* thread = (thread_t*)event->thread;
    list_t* eventlist = &thread->pendingSyncevents;

    lock_thread(current);

		// Having pending events
    if(!isListEmpty(eventlist)) {
		PRINF("During peek, calling singalCurrentThread %d. with pending events.\n", thread->index);
      // Signal itself when current event is first event of this thread.
      struct pendingSyncEvent* pe = NULL;

      // Search the whole list for given tid.
      pe = (struct pendingSyncEvent*)nextEntry(eventlist);
      while(true) {
        // We found this event
        if(pe->event == event) {
          PRINF("singalCurrentThread: signal myself thread %d, retrieve event %p pe->event %p", current->index, event, pe->event);
          // Remove this event from the list.
          listRemoveNode(&pe->list);

          // Release corresponding memory to avoid memory leakage.
          InternalHeap::getInstance().free(pe);

          // Now signal current thread.
          signalThread(thread);
    			
					PRINF("singalCurrentThread %d: event %p", thread->index, event);
          break;
        }

        // Update to the next thread.
        if(isListTail(&pe->list, eventlist)) {
          break;
        } else {
          pe = (struct pendingSyncEvent*)nextEntry(&pe->list);
        }
      } // while (true)
    } else {
      PRINF("signalCurrentThread %d: thread pending list is empty now!!!", thread->index);
    }
    
		unlock_thread(current);
  }

	/*  Peek the synchronization event (first event in the thread), it will confirm the following things
       1. Whether this event is expected event? If it is not, maybe it is caused by
          a race condition. Maybe we should restart the rollback or just simply reported this problem.
       2. Whether the first event is on the pending list, which means it is the thread's turn? 
          If yes, then the current signal thread should increment its semaphore.
       3. Whether the mutex_lock is successful or not? If it is not successful, 
          no need to wait for the semaphore since there is no actual lock happens.
	*/
  inline int peekSyncEvent(void* tlist) {
    int result = -1;
		PRINF("thread %d(%p): peekSyncEvent targetlist %p\n", current->index, current, tlist);	
		PRINF("thread %d syncevents %p: peekSyncEvent targetlist %p\n", current->index, &current->syncevents, tlist);	
    struct syncEvent* event = (struct syncEvent*)current->syncevents.getEntry();
		PRINT("thread %d: peek event %p targetlist %p\n", current->index, event, tlist);	
	
		// For debugging purpose. 	
		// If the event pointing to a different thread, or the target event 
		// is not the current one, warn about this situaion. Something wrong!
		if((event == NULL) || (event->thread != current) || (event->eventlist != tlist)) {
			PRINT("Assertion:peekSyncEvent at thread %d: event %p event thread %d. eventlist %p targetlist %p\n", current->index, event, ((thread_t*)event->thread)->index, event->eventlist, tlist);
			while(1) ;
			assert(event->thread == current);
			assert(event->eventlist == tlist);
		}

		// Signal current thread if the event is in the pending list   
    signalCurrentThread(event);

	  result = event->ret;
    return result;
  }

  void cleanSyncEvents() {
    // Remove all events in the global map and event list.
    syncvarsHashMap::iterator i;

    for(i = _syncvars.begin(); i != _syncvars.end(); i++) {
			struct SyncEntry* entry = (struct SyncEntry*)i.getData();
      SyncEventList* eventlist = (SyncEventList*)entry->list;

			PRINF("cleaningup the eventlist %p!!!\n", eventlist);
      eventlist->cleanup();
    }
  }

  inline void setSyncVariable(void** syncvariable, void* realvariable) {
    *syncvariable = realvariable;
  }

  /*
   Prepare rollback. Only one thread can call this function.
   It basically check every synchronization variable.
   If a synchronization variable is in the head of a thread, then
   we try to UP corresponding thread's semaphore.
   */
  void prepareRollback() {
    syncvarsHashMap::iterator i;
    struct SyncEntry* entry;
    void* syncvariable;

    for(i = _syncvars.begin(); i != _syncvars.end(); i++) {
      syncvariable = i.getkey();
			entry = (struct SyncEntry*)i.getData();

			// If syncvariable is not equal to the entry->realEntry, 
			// those are mutex locks, conditional variables or mutexlocks
			// The starting address of tose variables are cleaning up at epochBegin() (by backingup)
			// We have to make them to pointing to actual synchronization entries since there are not 
			// mutex_init or something else.
      if((*((void **)syncvariable)) != entry->realEntry) {
        // Setting the address
        setSyncVariable((void**)syncvariable, entry->realEntry);
			}
			
			PRINT("prepareRollback syncvariable %p pointintto %p entry %p realentry %p, eventlist %p\n", syncvariable, (*((void **)syncvariable)), entry, entry->realEntry, entry->list);

      prepareEventListRollback(entry->list);
    }
  }

  /*
    Prepare the rollback for an event list.
    It will check whether the first event in the event list is also the head of specific
    thread. If yes, then we will signal specific thread so that this thread can acquire
    the semaphore immediately.
   */
  inline void prepareEventListRollback(SyncEventList* eventlist) {
    struct syncEvent* event = eventlist->prepareRollback();

		PRINF("prepareEventListRollback eventlist %p event %p\n", eventlist, event);
    if(event) {
      // Signal to next thread with the top event
      signalNextThread(event);
    }
  }

  // Add one synchronization event into the pending list of a thread.
  void addPendingSyncEvent(struct syncEvent* event, thread_t* thread) {
    struct pendingSyncEvent* pendingEvent = NULL;

    pendingEvent = (struct pendingSyncEvent*)InternalHeap::getInstance().malloc(
        sizeof(struct pendingSyncEvent));

    listInit(&pendingEvent->list);
    pendingEvent->event = event;

    listInsertTail(&pendingEvent->list, &thread->pendingSyncevents);
  }

  // Check whether this event is the first event of corresponding thread.
  bool isThreadNextEvent(struct syncEvent* event, thread_t* thread) {
    return (event == thread->syncevents.firstIterEntry());
  }

  void signalThread(thread_t* thread) {
    semaphore* sema = &thread->sema;
    PRINF("Thread %d: ^^^^^Signal semaphore to thread %d\n", current->index, thread->index);
    sema->put();
  }

private:
  size_t getThreadSyncSeqNum() { return 0; }

  semaphore* getThreadSemaphore(thread_t* thread) { return &thread->sema; }

  // We are maintainning a private hash map for each thread.
  typedef HashMap<void*, struct SyncEntry*, spinlock, InternalHeapAllocator> syncvarsHashMap;

  // Synchronization related to different sync variable should be recorded into
  // the synchronization variable related list.
  syncvarsHashMap _syncvars;
};

#endif
