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
    void* realEntry;
    SyncEventList* list;
  };

public:
  xsync() {}

  void initialize() {
    _syncvars.initialize(HashFuncs::hashAddr, HashFuncs::compareAddr, xdefines::SYNCMAP_SIZE);
  }

  void insertSyncMap(void* key, void* realentry, SyncEventList* list) {
    struct SyncEntry* entry =
        (struct SyncEntry*)InternalHeap::getInstance().malloc(sizeof(struct SyncEntry));
    entry->realEntry = realentry;
    entry->list = list;
    _syncvars.insert(key, sizeof(key), entry);
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

    // Whether this event is on the top of thread?
    if(isThreadNextEvent(event, thread)) {
      // If yes, signal to this thread. There is no difference between
      // current thread or other thread.
      signalThread(thread);
      PRINF("Thread %d actually signal next thread %d on event %p", current->index, thread->index,
            event);
    } else {
      PRINF("Thread %d adding pending event to next thread %d on event %p", current->index,
            thread->index, event);
      addPendingSyncEvent(event, thread);
    }
  }

  // Signal current thread if event is one of pending events.
  void signalCurrentThread(struct syncEvent* event) {
    thread_t* thread = (thread_t*)event->thread;
    list_t* eventlist = &thread->pendingSyncevents;

    assert(thread == current);

    // PRINF("singalCurrentThread: event %p on variable %p command %d", event,
    // event->eventlist->getSyncVariable(), event->eventlist->getSyncCmd());

    if(!isListEmpty(eventlist)) {
      //     PRINF("singalCurrentThread: event %p thread %p, pending list is not empty!!!\n", event,
      // thread);
      // Only signal itself when current event is first event of this thread.
      struct pendingSyncEvent* pe = NULL;

      // Search the whole list for given tid.
      pe = (struct pendingSyncEvent*)nextEntry(eventlist);
      while(true) {
        // We found this event
        if(pe->event == event) {
          PRINF("singalCurrentThread: signal myself, retrieve event %p pe->event %p", event,
                pe->event);
          // Remove this event from the list.
          listRemoveNode(&pe->list);

          // Release corresponding memory to avoid memory leakage.
          InternalHeap::getInstance().free(pe);

          // Now signal current thread.
          signalThread(thread);
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
      PRINF("thread pending list is empty now!!!");
    }
  }

  // Update the synchronization list.
  void advanceThreadSyncList() {
    struct syncEvent* nextEvent = NULL;

    global_lock();

    // Update next event of thread eventlist.
    nextEvent = current->syncevents.nextIterEntry();
    if(nextEvent != NULL) {
      signalCurrentThread(nextEvent);
    }
    global_unlock();
  }

  // peekSyncEvent return the saved event value for current synchronization.
  inline int peekSyncEvent() {
    int result = -1;
    struct syncEvent* event = (struct syncEvent*)current->syncevents.getEntry();
    if(event) {
      REQUIRE(event->thread == current,
              "Event %p belongs to thread %p, not the current thread (%p)", event, event->thread,
              current);
      result = event->ret;
    } else {
      // ERROR("Event not exising now at thread %p!!!!\n", current);
    }
    return result;
  }

  void cleanSyncEvents() {
    // Remove all events in the global map and event list.
    syncvarsHashMap::iterator i;

    for(i = _syncvars.begin(); i != _syncvars.end(); i++) {
      SyncEventList* eventlist = (SyncEventList*)i.getData();
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
    SyncEventList* eventlist = NULL;
    void* syncvariable;

    for(i = _syncvars.begin(); i != _syncvars.end(); i++) {
      syncvariable = i.getkey();
      entry = (struct SyncEntry*)i.getData();
      if(syncvariable != entry) {
        // Setting the address
        setSyncVariable((void**)syncvariable, entry->realEntry);
      }

      eventlist = entry->list;
      prepareEventListRollback(eventlist);
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

    // Add this pending event into corresponding thread.
    listInsertTail(&pendingEvent->list, &thread->pendingSyncevents);
  }

  // Check whether this event is the first event of corresponding thread.
  bool isThreadNextEvent(struct syncEvent* event, thread_t* thread) {
    return (event == thread->syncevents.firstIterEntry());
  }

  void signalThread(thread_t* thread) {
    semaphore* sema = &thread->sema;
    PRINF("Signal semaphore to thread%d (at %p)\n", thread->index, thread);
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
