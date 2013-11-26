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
 * @file   callsite.h
 * @brief  Management about callsite for heap objects.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */ 
    
#ifndef _CALLSITE_H
#define _CALLSITE_H

#include <link.h>
#include <stdio.h>

#include "xdefines.h"

class CallSite {
public:
  CallSite()
  : _depth(0)
  {

  }

  unsigned long depth() 
  {
    return _depth;
  }

  void print()
  {
    for(int i = 0; i < _depth; i++) {
      printf("%p\t", _callsite[i]);
    }
    printf("\n");
  }

  unsigned long get(int index) {
    return (unsigned long)_callsite[index];
  }

  // Save callsite
  void save(int depth, void ** addr) {
    _depth = depth;

    for(int i = 0; i < depth; i++) {
      _callsite[i] = addr[i];
    }
  }

private:
  unsigned long _depth; // Actual callsite depth
  void * _callsite[xdefines::CALLSITE_MAXIMUM_LENGTH];
};

#endif
