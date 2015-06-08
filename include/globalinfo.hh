#if !defined(DOUBLETAKE_GLOBALINFO_H)
#define DOUBLETAKE_GLOBALINFO_H

/*
 * @file   globalinfo.h
 * @brief  some global information about this system, by doing this, we can avoid multiple copies.
 *         Also, it is very important to utilize this to cooperate multiple threads since
 * pthread_kill
 *         actually can not convey additional signal value information.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

#include "log.hh"
#include "real.hh"
#include "threadstruct.hh"

enum SystemPhase {
  E_SYS_INIT,        // Initialization phase
  E_SYS_EPOCH_END,   // We are just before commit.
  E_SYS_EPOCH_BEGIN, // We have to start a new epoch when no overflow.
};
extern bool g_isRollback;
extern bool g_hasRollbacked;
extern int g_numOfEnds;
extern enum SystemPhase g_phase;
extern pthread_cond_t g_condCommitter;
extern pthread_cond_t g_condWaiters;
extern pthread_mutex_t g_mutex;
extern pthread_mutex_t g_mutexSignalhandler;
extern int g_waiters;
extern int g_waitersTotal;

inline void global_lock() { Real::pthread_mutex_lock(&g_mutex); }

inline void global_unlock() { Real::pthread_mutex_unlock(&g_mutex); }

inline void global_lockInsideSignalhandler() { Real::pthread_mutex_lock(&g_mutexSignalhandler); }

inline void global_unlockInsideSignalhandler() {
  Real::pthread_mutex_unlock(&g_mutexSignalhandler);
}

inline void global_initialize() {
  g_isRollback = false;
  g_hasRollbacked = false;
  g_phase = E_SYS_INIT;
  g_numOfEnds = 0;

  Real::pthread_mutex_init(&g_mutex, NULL);
  Real::pthread_mutex_init(&g_mutexSignalhandler, NULL);
  Real::pthread_cond_init(&g_condCommitter, NULL);
  Real::pthread_cond_init(&g_condWaiters, NULL);
}

inline void global_setEpochEnd() {
  g_numOfEnds++;
  g_phase = E_SYS_EPOCH_END;
}

inline bool global_isInitPhase() { return g_phase == E_SYS_INIT; }

inline bool global_isEpochEnd() { return g_phase == E_SYS_EPOCH_END; }

inline bool global_isRollback() { return g_isRollback; }

inline bool global_isEpochBegin() { return g_phase == E_SYS_EPOCH_BEGIN; }

inline void global_setRollback() {
  g_isRollback = true;
  g_hasRollbacked = true;
}

inline bool global_hasRollbacked() { return g_hasRollbacked; }

inline void global_wakeup() {
  // Wakeup all other threads.
  Real::pthread_cond_broadcast(&g_condWaiters);
}

inline void global_epochBegin() {
  global_lockInsideSignalhandler();

  g_phase = E_SYS_EPOCH_BEGIN;
  PRINF("waken up all waiters\n");
  // Wakeup all other threads.
  Real::pthread_cond_broadcast(&g_condWaiters);

  if(g_waiters != 0) {
    Real::pthread_cond_wait(&g_condCommitter, &g_mutexSignalhandler);
  }
  global_unlockInsideSignalhandler();
}

inline thread_t* global_getCurrent() { return current; }

// Waiting for the stops of threads, no need to hold the lock.
inline void global_waitThreadsStops(int totalwaiters) {
  global_lockInsideSignalhandler();
  g_waitersTotal = totalwaiters;
  //    PRINF("During waiting: g_waiters %d g_waitersTotal %d\n", g_waiters, g_waitersTotal);
  while(g_waiters != g_waitersTotal) {
    Real::pthread_cond_wait(&g_condCommitter, &g_mutexSignalhandler);
  }
  global_unlockInsideSignalhandler();
}

inline void global_checkWaiters() { 
//	assert(g_waiters == 0); 
	if(g_waiters != 0) {
		PRINT("g_waiters is %d\n", g_waiters);
		while(1) ;
	}
}

// Notify the commiter and wait on the global conditional variable
inline void global_waitForNotification() {
  assert(global_isEpochEnd() == true);

  //    printf("waitForNotification, waiters is %d at thread %p\n", g_waiters, pthread_self());
  global_lockInsideSignalhandler();
  //PRINF("waitForNotification g_waiters %d totalWaiters %d\n", g_waiters, g_waitersTotal);
  g_waiters++;

	// Wakeup the committer
  if(g_waiters == g_waitersTotal) {
    Real::pthread_cond_signal(&g_condCommitter);
  }

  // Only waken up when it is not the end of epoch anymore.
  while(global_isEpochEnd()) {
    PRINF("waitForNotification before waiting again\n");
    Real::pthread_cond_wait(&g_condWaiters, &g_mutexSignalhandler);
    PRINF("waitForNotification after waken up. isEpochEnd() %d \n", global_isEpochEnd());
  }

  g_waiters--;

  if(g_waiters == 0) {
    Real::pthread_cond_signal(&g_condCommitter);
  }

  global_unlockInsideSignalhandler();
}

#endif
