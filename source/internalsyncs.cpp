/*
 * @file   internal.cpp
 * @brief  Managing internal synchronizations used by StopGap itself.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#include "internalsyncs.hh"

#include "real.hh"
#include "threadstruct.hh"

void lock_thread(thread_t* thread) { Real::pthread_mutex_lock(&thread->mutex); }

void unlock_thread(thread_t* thread) { Real::pthread_mutex_unlock(&thread->mutex); }

void wait_thread(thread_t* thread) { Real::pthread_cond_wait(&thread->cond, &thread->mutex); }

void signal_thread(thread_t* thread) { Real::pthread_cond_broadcast(&thread->cond); }
