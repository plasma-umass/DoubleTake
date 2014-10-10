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
  current->qlist.restore();
  current->syncevents.cleanup();
}

void xthread::prepareRollback() {
  PRINF("calling syscalls::prepareRollback\n");
  // System call should be rolled back before memory rollback.
  // syscalls::getInstance().prepareRollback();

  PRINF("calling threadmap::prepareRollback\n");
  // Initialize the semaphores for threads at first since preparing synchronization may
  // increment corresponding semaphore.
  threadmap::getInstance().prepareRollback();

  PRINF("calling xsync::prepareRollback\n");
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

bool xthread::addQuarantineList(void* ptr, size_t sz) {
  return current->qlist.addFreeObject(ptr, sz);
}

void xthread::rollback() {
  current->qlist.restore();

  //	PRINT("xthread::rollback now\n");
  // Recover the context for current thread.
  restoreContext();
}
