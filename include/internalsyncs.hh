#if !defined(DOUBLETAKE_INTERNALSYNCS_H)
#define DOUBLETAKE_INTERNALSYNCS_H

/*
 * @file   internalsyncs.h
 * @brief  Managing internal synchronizations 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "threadstruct.hh"

void lock_thread(thread_t* thread);
void unlock_thread(thread_t* thread);
void wait_thread(thread_t* thread);
void signal_thread(thread_t* thread);

#endif
