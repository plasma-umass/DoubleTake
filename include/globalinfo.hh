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

extern pthread_mutex_t g_mutex;

inline void global_lock() { Real::pthread_mutex_lock(&g_mutex); }
inline void global_unlock() { Real::pthread_mutex_unlock(&g_mutex); }

inline void global_initialize() {
  Real::pthread_mutex_init(&g_mutex, NULL);
}

#endif
