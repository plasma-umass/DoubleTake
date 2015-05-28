/*
 * @file   xthread.cpp
 * @brief  Handle Thread related information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xthread.hh"

#include "globalinfo.hh"
#include "internalsyncs.hh"
#include "list.hh"
#include "log.hh"
#include "quarantine.hh"
#include "recordentries.hh"
#include "threadinfo.hh"
#include "threadmap.hh"
#include "threadstruct.hh"
#include "xrun.hh"
#include "syscalls.hh"
#include "xsync.hh"

// Global lock used when joining and exiting a thread.
// threadmap::threadHashMap threadmap::_xmap;
// list_t threadmap::_alivethreads;

// bool xthread::_isRollbackPhase;
// list_t xthread::_deadSyncVars;
__thread thread_t* current;
// threadmap::threadHashMap threadmap::_xmap;
list_t threadmap::_alivethreads;

int getThreadIndex() {
  if(!global_isInitPhase()) {
    return current->index;
  } else {
    return 0;
  }
}

char* getThreadBuffer() {
  int index = getThreadIndex();

  return threadinfo::getInstance().getThreadBuffer(index);
}

void xthread::invokeCommit() {
  xrun::getInstance().epochEnd(false);
  xrun::getInstance().epochBegin();
}

void xthread::epochBegin() {
	// Now we should not have the pending synchronization events.	
	listInit(&current->pendingSyncevents);

	// Handle the quarantine list of memory.
  current->qlist.restore();

	//PRINF("Cleanup all synchronization events for this thread\n");
	// Handle the synchronization events
  current->syncevents.cleanup();
	//PRINF("Cleanup all synchronization events for this thread done\n");
}

void xthread::prepareRollback() {
  PRINF("calling syscalls::prepareRollback\n");
  PRINT("calling threadmap::prepareRollback\n");
  // Initialize the semaphores for threads at first since preparing synchronization may
  // increment corresponding semaphore.
	// Also, prepare the rollback of system call records.
  threadmap::getInstance().prepareRollback();

  PRINF("calling xsync::prepareRollback\n");
  PRINT("before calling sync::prepareRollback\n");
  // Update the semaphores and synchronization entry
  _sync.prepareRollback();
  PRINT("after calling sync::prepareRollback\n");
}

void xthread::setThreadSafe() {
  lock_thread(current);
  current->isSafe = true;

  if(current->waitSafe) {
    signal_thread(current);
  }
  unlock_thread(current);
}

bool xthread::addQuarantineList(void* ptr, size_t sz) {
  return current->qlist.addFreeObject(ptr, sz);
}

void xthread::rollback() {
  current->qlist.restore();

  PRINT("xthread::rollback now\n");
  // Recover the context for current thread.
  restoreContext();
}
