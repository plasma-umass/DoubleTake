// -*- C++ -*-

/*
  Copyright (c) 2008-12, University of Massachusetts Amherst.

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
 * @file   xglobals.h
 * @brief  Management of all globals, including the globals of applications, libc.so and libpthread.so.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *         We adopted this from sheriff project, but we do substantial changes here.
 */

#ifndef _XGLOBALS_H_
#define _XGLOBALS_H_

#include "xdefines.h"
#include "xmapping.h"
#include "selfmap.h"
#include "regioninfo.h"

/// @class xglobals
/// @brief Maps the globals region onto a persistent store.
class xglobals {
public:
  
  
  xglobals ()
    : _numbRegions(0)
  {
	 // fprintf(stderr, "GLOBALS_START is %x\n", GLOBALS_START);
  }

  void initialize(void) {
    // We check those mappings to find out existing globals.
    int numb;
    int i;
    
    // Trying to get information about different text segmensts.
    selfmap::getInstance().getTextRegions();

    // Trying to get the information of global regions
    selfmap::getInstance().getGlobalRegions(&_regions[0], &_numbRegions);

    // Do the initialization for each global.
    for(int i = 0; i < _numbRegions; i++) {
//      PRWRN("Call begin at i %d from %p to %p\n", i, _regions[i].start, _regions[i].end); 
      printf("Call begin at i %d from %p to %p\n", i, _regions[i].start, _regions[i].end); 
      _maps[i].initialize(_regions[i].start, (size_t)((intptr_t)_regions[i].end - (intptr_t)_regions[i].start));
    }

    //while(1); 
  }

  void finalize(void) {
    // Nothing need to do here.
  }

  void recoverMemory(void) {
    for(int i = 0; i < _numbRegions; i++) {
      //fprintf(stderr, "Call begin at i %d from %p to %p\n", i, _regions[i].start, _regions[i].end); 
      _maps[i].recoverMemory(NULL);
    }

  }

  // Commit all regions in the end of each transaction.
  void backup(void) {
    for(int i = 0; i < _numbRegions; i++) {
      _maps[i].backup(NULL);
    }
  }

  void commit(void * start, size_t size, int index) {
    _maps[index].commit(start, size);
  }

private:
  int _numbRegions; // How many blocks actually.
  regioninfo _regions[xdefines::NUM_GLOBALS];
  xmapping _maps[xdefines::NUM_GLOBALS];
};

#endif
