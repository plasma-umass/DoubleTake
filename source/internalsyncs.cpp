// -*- C++ -*-
/*
  Copyright (c) 2012, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   internal.cpp
 * @brief  Managing internal synchronizations used by StopGap itself.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#include "real.h"
#include "internalsyncs.h"

void lock_thread(thread_t * thread) {
  Real::pthread_mutex_lock(&thread->mutex); 
}

void unlock_thread(thread_t* thread) {
  Real::pthread_mutex_unlock(&thread->mutex); 
}

void wait_thread(thread_t*thread) {
  Real::pthread_cond_wait(&thread->cond, &thread->mutex); 
}

void signal_thread(thread_t*thread) {
  Real::pthread_cond_broadcast(&thread->cond); 
}
