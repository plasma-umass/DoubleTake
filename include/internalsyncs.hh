#if !defined(DOUBLETAKE_INTERNALSYNCS_H)
#define DOUBLETAKE_INTERNALSYNCS_H

/*
 * @file   internalsyncs.h
 * @brief  Managing internal synchronizations
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "threadstruct.hh"

void lock_thread(DT::Thread* thread);
void unlock_thread(DT::Thread* thread);
void wait_thread(DT::Thread* thread);
void signal_thread(DT::Thread* thread);

#endif
