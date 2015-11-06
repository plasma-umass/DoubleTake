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
#include "doubletake.hh"

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

inline bool global_isEpochBegin() { return g_phase == E_SYS_EPOCH_BEGIN; }

inline bool global_hasRollbacked() { return g_hasRollbacked; }

inline thread_t* global_getCurrent() { return current; }

#endif
