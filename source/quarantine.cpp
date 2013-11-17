// -*- C++ -*-
/*
Copyright (c) 2012, University of Massachusetts Amherst.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

/*
* @file quarantine.cpp
* @brief Manage those quarantine objects.
         Those objects are freed in a FIFO order when 
         the slots is not enough to hold objects or total size is too large.
         Whichever comes first, we will evict one object. 
* @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
*/
#include "xdefines.h"
#include "xmemory.h"
#include "quarantine.h"

void quarantine::realfree(void * ptr) {
  // Calling actual heap object to free this object.
  xmemory::getInstance().realfree(ptr);
}

void quarantine::rollback(void){
  // Calling the rollback.
  xmemory::getInstance().rollback();
}

