// -*- C++ -*-

/*
  Copyright (C) 2011 University of Massachusetts Amherst.

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
 * @file   xmemory.cpp
 * @brief  Memory management for all.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 
#include "xrun.h"
#include "xmemory.h"

xpheap<xoneheap<xheap > > xmemory::_pheap;
 
// This function is called inside the segmentation fault handler
// So we must utilize the "context" to achieve our target 
void xmemory::handleSegFault(void)
{
#ifdef DETECT_OVERFLOW
  fprintf(stderr, "Returning from segmentation fault error\n"); 
  // Check whether the segmentation fault is called by buffer overflow.
  if(checkHeapOverflow()) {
    // Now we can roll back 
    fprintf(stderr, "\n\nOVERFLOW causes segmenation fault!!!! ROLLING BACK\n\n\n");
    
    xrun::getInstance().rollback();
  }  
  else {
    fprintf(stderr, "\n\nNO overflow in segmenation fault, ROLLING BACK and stop\n\n\n");
    // We may rely on gdb to find out this problem.
    // We do not need to install watchpoints at all.
    // But we can rollback everything and hault there
    // so gdb can be used to attach to this process.
    xrun::getInstance().rollbackandstop();
  }
#else 
  xrun::getInstance().rollbackandstop();
#endif
}
 
void xmemory::cleanupFreeList(void) {
  freelist::getInstance().postFreeAllObjects(); 
}
 
void xmemory::freeAllObjects(void)
{
  freelist::getInstance().preFreeAllObjects(); 
  struct freeObject * obj = NULL;

  while(obj = freelist::getInstance().retrieveFreeObject()) {
    _pheap.realfree(obj->ptr, obj->owner);
  }
 
  freelist::getInstance().postFreeAllObjects(); 
//  _pheap.realfree(ptr, tindex);
}
