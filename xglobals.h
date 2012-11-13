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

    // Trying to get the information of global regions
    selfmap::getGlobalRegions(&_regions[0], &_numbRegions);

    // Do the initialization for each global.
    for(int i = 0; i < _numbRegions; i++) {
   // for(int i = 1; i < _numbRegions; i++) {
      _maps[i].initialize(_regions[i].start, 
                          (size_t)((intptr_t)_regions[i].end - (intptr_t)_regions[i].start));
    }
  }

  void finalize(void) {
    // Nothing need to do here.
  }

  // Handling the protection for all regions.
  void openProtection(void) {
    for(int i = 0; i < _numbRegions; i++) {
      _maps[i].openProtection();
    }
  }

  void closeProtection(void) {
    for(int i = 0; i < _numbRegions; i++) {
      _maps[i].closeProtection();
    }
  }


  // Release all temporary pages of all regions in the begin of each transaction.
  void begin(void) {
    for(int i = 0; i < _numbRegions; i++) {
      _maps[i].begin();
    }
  }

  // Check and commit all regions in the end of each transaction.
  void checkandcommit(void) {
    for(int i = 0; i < _numbRegions; i++) {
      _maps[i].checkandcommit();
    }
  }

  // Check whether the address is inside the 
  // range of globals. We are using the index to identify
  // which region of globals since there are multiple global regions.
  bool inRange(void * addr, int * index) {
    int i;
    bool isFound = false;

    // Check whether the given address is in one of regions.
    for(i = 0; i < _numbRegions; i++)  {
      if(addr >= _regions[i].start && addr <= _regions[i].end) {
        *index = i;
        isFound = true;
        break;
      } 
    } 
  
  //  printf("addr %p in index %d\n", addr, *index);
    return isFound;
  }

  void handleWrite(void * addr, int index) {
    //printf("addr %p is in index %d\n", addr, index);
    _maps[index].handleWrite(addr);
  }


  unsigned long sharemem_read_word(void * dest, int index) {
    return _maps[index].sharemem_read_word(dest);
  }

  void sharemem_write_word(void * dest, unsigned long val, int index) {
    return _maps[index].sharemem_write_word(dest, val);
  }

private:
  int _numbRegions; // How many blocks actually.
  regioninfo _regions[xdefines::NUM_GLOBALS];
  xmapping _maps[xdefines::NUM_GLOBALS];
};

#endif
