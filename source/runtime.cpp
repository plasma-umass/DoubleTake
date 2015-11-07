/**
 * @file  runtime.cpp
 * @brief Global functions not tied to the global initialization of
 *        libdoubletake.
 */

#include "doubletake.hh"
#include "xrun.hh"

namespace doubletake {
  std::atomic_bool initialized;
  std::atomic_bool inRollback;
}

static std::atomic_int EPOCH_ID;
static std::atomic_int WAITING_COUNT;

// used to make sure all threads have quiesced, and only the
// coordinator is running.
static pthread_cond_t coordinator_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t coordinator_lock = PTHREAD_MUTEX_INITIALIZER;

// used by all the other threads to wait until end-of-epoch integrity
// checking has finished.
static pthread_cond_t epoch_completed_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t epoch_completed_lock = PTHREAD_MUTEX_INITIALIZER;

// the global runtime lock
static pthread_mutex_t rtLock = PTHREAD_MUTEX_INITIALIZER;

void doubletake::unblockEpochSignal() {
  sigset_t blocked;

  sigemptyset(&blocked);
  sigaddset(&blocked, SIGUSR2);

  Real::sigprocmask(SIG_UNBLOCK, &blocked, nullptr);
}

void doubletake::lock() {
  sigset_t blocked, current;
  struct timespec sleep;
  sleep.tv_sec = 0;
  sleep.tv_nsec = 10000000; // 1/100 of a second

  sigemptyset(&blocked);
  sigaddset(&blocked, SIGUSR2);

  // FIXME: I think we need to make sure we're not interrupted in our
  // call to pthread_mutex_(try)lock, because if we are and rollback
  // it could corrupt the state of the mutex, causing deadlocks.  This
  // could probably be done better.  Maybe we could use SIGUSR1?
  while (true) {
    Real::sigprocmask(SIG_BLOCK, &blocked, &current);

    // a return value of 0 means we acquired the lock, great!
    if (!Real::pthread_mutex_trylock(&rtLock))
      break;

    // at this point a pending SIGUSR2 signal will be delivered if
    // someone else is trying to end the epoch
    Real::sigprocmask(SIG_SETMASK, &current, nullptr);
    Real::nanosleep(&sleep, nullptr);
  }
}

void doubletake::unlock() {
  Real::pthread_mutex_unlock(&rtLock);
  doubletake::unblockEpochSignal();
}

void doubletake::setWaiterCount(size_t n) {
  WAITING_COUNT = n;
}

void doubletake::waitUntilQuiescent() {
  Real::pthread_mutex_lock(&coordinator_lock);
  while (WAITING_COUNT)
    Real::pthread_cond_wait(&coordinator_cond, &coordinator_lock);
  Real::pthread_mutex_unlock(&coordinator_lock);
}

int doubletake::currentIsQuiesced() {
  int id = EPOCH_ID;
  Real::pthread_mutex_lock(&coordinator_lock);
  // if we're the last thread being waited on at the end of an epoch,
  // wake up the coordinator.
  if (WAITING_COUNT.fetch_sub(1) == 1)
    Real::pthread_cond_broadcast(&coordinator_cond);
  Real::pthread_mutex_unlock(&coordinator_lock);
  return id;
}

void doubletake::epochComplete() {
  Real::pthread_mutex_lock(&epoch_completed_lock);
  EPOCH_ID++;
  Real::pthread_cond_broadcast(&epoch_completed_cond);
  Real::pthread_mutex_unlock(&epoch_completed_lock);
}

void doubletake::waitForEpochComplete(const int id) {
  Real::pthread_mutex_lock(&epoch_completed_lock);
  // FIXME: this serializes the wakeup of all threads, as only 1 can
  // hold epoch_completed_lock at any given time.  Maybe use a barrier
  // instead?
  while (EPOCH_ID <= id)
    Real::pthread_cond_wait(&epoch_completed_cond, &epoch_completed_lock);
  Real::pthread_mutex_unlock(&epoch_completed_lock);
}

int doubletake::epochID() {
  return EPOCH_ID;
}


int doubletake::findStack(pid_t tid, uintptr_t *bottom, uintptr_t *top) {
  return xrun::getInstance().findStack(tid, bottom, top);
}

void doubletake::printStackCurrent() {
  return xrun::getInstance().printStackCurrent();
}

void doubletake::printStack(size_t len, void **frames) {
  return xrun::getInstance().printStack(len, frames);
}

bool doubletake::isLib(void *pcaddr) {
  return xrun::getInstance().isDoubleTake(pcaddr);
}
