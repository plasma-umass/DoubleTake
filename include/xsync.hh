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

// deferSync should be implemented here, not threadinfo.hh
// However, those thread spawning or joinning should be kept at threainfo.hh.
// When a synchronization variable has been destroyed, we should not invoke another 
// malloc. Thus, 
class xsync {
public:
  struct SyncEntry {
		list_t listactive; 
		list_t listnew;
		list_t listdel; 
		syncVariableType type; 
    void * nominal;
		void * real;
    SyncEventList* syncevents;
  };

private:
	class NewSyncvarList {
		public:
			void initialization(void) {
				listInit(&_list);
			}

			void insertTail(struct SyncEntry * entry) {
				listInsertTail(&entry->listnew, &_list);
			}

			struct SyncEntry * getSyncEntry(list_t * listentry) {
				return (struct SyncEntry *)((intptr_t)listentry - sizeof(list_t)); 	
			}

			// Reinitialize the list in the epochBegin.
			// We don't care about original entries in the list since those 
			// entries are also in the other list, which will be handled correpondingly.
			void cleanup(void) {
				listInit(&_list);
			}

			// Retrieve the first entry in the list (after _currentry);
			// Since we would like to support the endless replays, thus, whenever
			// we find an entry, we will move the current entry into the tail of the list.
			// Thus, we hope that we will have the same list after one round.
			struct SyncEntry * retrieveSyncEntry(syncVariableType type, void * nominal) {
				struct SyncEntry * entry = NULL;
				struct SyncEntry * target = NULL;
				list_t * last = _list.prev;
				list_t * next = NULL;

				// If the list is empty, trigger an assertion.
				if(isListEmpty(&_list)) {
					PRINF("the listnew should not be empty!\n");
					assert(0);
				}	

				// Now we will search the whole list from the beginning.
				next = _list.next;

				while(true) {
					// Get the synchronization entry according to list pointer.
					entry = getSyncEntry(next);

					// Check whether the current entry is what we want. 
					// In most cases, it is the truth. However, if two threads are initiating two 
					// different variables in the same epoch, those initialization may be invoked in
					// a different order in replay phase. We should handle that as well.
					if(entry->type == type && entry->nominal == nominal) {
						target = entry;
				
						// remove current node.
						listRemoveNode(&entry->listnew);

						// Insert this node into the end of the list.
						insertTail(target);	
						break;
					}

					// If we have searched the whole list, we will quit as well.
					if(next == last) {
						break;
					}
					
					// Otherwise, we will check the next one.
					next = next->next;
				}

				assert(target != NULL);
				
				return target;
			}

		private:
			list_t  _list;
	};

public:
  xsync() {}

	static xsync& getInstance() {
    static char buf[sizeof(xsync)];
    static xsync* theOneTrueObject = new (buf) xsync();
    return *theOneTrueObject;
  }


  void initialize() {
		// Initialize two lists for record synchronization variables.
 		listInit(&_activeList);
 		listInit(&_delList);

		_newList.initialization();
		
		Real::pthread_mutex_init(&_mutex, NULL);
  }

  struct SyncEntry * recordSyncVar(syncVariableType type, void* nominal, void* real, SyncEventList* list) {
		// Alloc a synchronization variable entry.
    struct SyncEntry* entry =
        (struct SyncEntry*)InternalHeap::getInstance().malloc(sizeof(struct SyncEntry));
		entry->type = type;
    entry->real = real;
    entry->nominal = nominal;
    entry->syncevents = list;

		lock();

		// record this synchronization variables on two lists
    listInsertTail(&entry->listactive, &_activeList);

		// Insert this into new list.
		_newList.insertTail(entry);

		unlock();

		return entry;
  }

	void freeSyncEntry(struct SyncEntry * entry) {
		InternalHeap::getInstance().free((void *)entry);
	}

	void deferSync(struct SyncEntry *entry) {
		lock();
		listInsertTail(&entry->listdel, &_delList);
		unlock();
	}

	struct SyncEntry * getSyncEntryDellist(list_t * listentry) {
		return (struct SyncEntry *)((intptr_t)listentry - sizeof(list_t) - sizeof(list_t));
	}

	void removeFromDellist(struct SyncEntry * entry) {
		listRemoveNode(&entry->listdel);
	}
	
	void removeFromAlllist(struct SyncEntry * entry) {
		listRemoveNode(&entry->listactive);
	}

	// In the rollback phase, we will fetch synchronization entry 
	// during the initialization of every sync variable
	void * retrieveRealSyncEntry(syncVariableType type, void * nominal) {
		struct SyncEntry * entry = NULL;
		lock();
		entry =  _newList.retrieveSyncEntry(type, nominal);
		unlock();
		if(entry != NULL) {
			return entry->real;
		}
		else {
			return NULL;
		}	
	}

	// During the begin of an epoch
	void epochBegin(void) {
		struct SyncEntry * entry = NULL;
		list_t * last = NULL;
		list_t * next = NULL;
		list_t * now = NULL;

		// Cleanup every entry in _delList.
		if(!isListEmpty(&_delList)) {
			// Now we will search the whole list from the beginning.
			last = _delList.prev;
			now = _delList.next;

			while(true) {
				next = now->next;

				// Get the synchronization entry according to list pointer.
				entry = getSyncEntryDellist(now);

				// We will remove the entry from deleted list and activelist.
				removeFromDellist(entry);
				removeFromAlllist(entry);

				// Now we can remove this entry.
				freeSyncEntry(entry);
	
				if(now == last) {
					break;
				}

				// Get the next entry;
				now = next;
			}
		}

		// Cleanup all entries in the _newList
		// there is no new entries in the beginning of an epoch 
		_newList.cleanup();

		// Updating the eventlist for all existing synchronization variables 
		if(!isListEmpty(&_activeList)) {
			// Now we will search the whole list from the beginning.
			last = _activeList.prev;
			now = _activeList.next;

			while(true) {
				next = now->next;

				// Get the synchronization entry according to list pointer.
				entry = (struct SyncEntry *)now;

				// Cleanup the sync events on this synchronization variable since
				// we will record new ones in the new epoch
      	entry->syncevents->cleanup();

				if(now == last) {
					break;
				}
				else {
					now = next;
				}
			}
		}

	}

  // Signal next thread on the same synchronization variable.
  static void signalNextThread(struct syncEvent* event) {
    thread_t* thread = (thread_t*)event->thread;

		// Acquire the lock before adding an event to the pending list
		// since the thread may try to check whether it can proceed or not
    lock_thread(thread);

    // Add this pending event into corresponding thread.
    // Whether this event is on the top of thread?
    if(isThreadNextEvent(event, thread)) {
      // If yes, signal to this thread. There is no difference between
      // current thread or other thread.
      PRINF("Thread %d actually signal next thread %d on event %p", current->index, thread->index, event);
      signalThread(thread);
    } else {
      PRINF("Thread %d adding pending event to next thread %d on event %p", current->index,
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
		PRINF("thread %d: peek event %p targetlist %p\n", current->index, event, tlist);	
	
		// For debugging purpose. 	
		// If the event pointing to a different thread, or the target event 
		// is not the current one, warn about this situaion. Something wrong!
		if((event == NULL) || (event->thread != current) || (event->eventlist != tlist)) {
			PRINF("Assertion:peekSyncEvent at thread %d: event %p event thread %d. eventlist %p targetlist %p\n", current->index, event, ((thread_t*)event->thread)->index, event->eventlist, tlist);
			while(1) ;
			assert(event->thread == current);
			assert(event->eventlist == tlist);
		}

		// Signal current thread if the event is in the pending list   
    signalCurrentThread(event);

	  result = event->ret;
    return result;
  }

  /*
   Prepare rollback. Only one thread can call this function.
   It basically check every synchronization variable.
   If a synchronization variable is in the head of a thread, then
   we try to UP corresponding thread's semaphore.
   */
  void prepareRollback() {
    struct SyncEntry* entry;
		list_t * next;
		list_t * last = _activeList.prev;

		// If list is empty, return immediately.
		if(isListEmpty(&_activeList)) {
			return;
		}
		
		next = _activeList.next;
		while(true) {
			entry = (struct SyncEntry*)next;
			PRINF("prepareRollback syncvariable %p pointintto %p entry %p, eventlist %p\n", entry->nominal, entry->real, entry, entry->syncevents);

      prepareEventListRollback(entry->syncevents);
		
			// If we have traverse the whole list, let's quit now.	
			if(next == last) {
				break;
			}

			// Check the next entry;
			next = next->next;
    }
  }

  /*
    Prepare the rollback for an event list.
    It will check whether the first event in the event list is also the head of specific
    thread. If yes, then we will signal specific thread so that this thread can acquire
    the semaphore immediately.
   */
  static void prepareEventListRollback(SyncEventList* eventlist) {
    struct syncEvent* event = eventlist->prepareRollback();

		PRINF("prepareEventListRollback eventlist %p event %p\n", eventlist, event);
    if(event) {
      // Signal to next thread with the top event
      signalNextThread(event);
    }
  }

  // Add one synchronization event into the pending list of a thread.
  static void addPendingSyncEvent(struct syncEvent* event, thread_t* thread) {
    struct pendingSyncEvent* pendingEvent = NULL;

    pendingEvent = (struct pendingSyncEvent*)InternalHeap::getInstance().malloc(
        sizeof(struct pendingSyncEvent));

    listInit(&pendingEvent->list);
    pendingEvent->event = event;

    listInsertTail(&pendingEvent->list, &thread->pendingSyncevents);
  }

  // Check whether this event is the first event of corresponding thread.
  static bool isThreadNextEvent(struct syncEvent* event, thread_t* thread) {
    return (event == thread->syncevents.firstIterEntry());
  }

  static void signalThread(thread_t* thread) {
    semaphore* sema = &thread->sema;
    PRINF("Thread %d: ^^^^^Signal semaphore to thread %d\n", current->index, thread->index);
    sema->put();
  }

private:

	void lock(void) {
		Real::pthread_mutex_lock(&_mutex);
	}

	void unlock(void) {
		Real::pthread_mutex_unlock(&_mutex);
	}

	pthread_mutex_t _mutex;
	
	// We are keeping track of three different list.
	// One is the new list, the second one is delete list.
	// The other one is alllist.
	NewSyncvarList _newList;
	
	// Active list for synchronization variables.
	list_t  _activeList;
	list_t  _delList;
};

#endif
