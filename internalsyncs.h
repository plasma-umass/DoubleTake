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
 * @file   internalsyncs.h
 * @brief  Managing internal synchronizations 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef _INTERNALSYNCS_H_
#define _INTERNALSYNCS_H_
#include<pthread.h>

extern "C" {

extern int internal_mutex_lock(pthread_mutex_t*);
extern int internal_mutex_unlock(pthread_mutex_t*);
extern int internal_cond_wait(pthread_cond_t*, pthread_mutex_t*);
extern int internal_cond_signal(pthread_cond_t*);
extern int internal_cond_broadcast(pthread_cond_t*);

};

#endif
