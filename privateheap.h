// -*- C++ -*-

/*
  Copyright (c) 2008-12 , University of Massachusetts Amherst.

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

#ifndef _PRIVATEHEAP_H_
#define _PRIVATEHEAP_H_
#include "stdio.h"
#include "string.h"
/**
 * @file PrivateHeap.h
 * @brief A heap for Grace's internal allocation needs.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *
 */

extern "C" {
  void * dlmalloc (size_t sz);
  void dlfree (void *);
  size_t dlmalloc_usable_size (void *);
  void * dlrealloc(void * ptr, size_t sz);
}

class PrivateHeap {
public:

  virtual ~PrivateHeap (void) {}

  static void * malloc (size_t sz) {
    return dlmalloc (sz);
  }

  static void free (void * ptr) {
    dlfree (ptr);
  }

  static size_t getSize (void * ptr) {
    return dlmalloc_usable_size (ptr);
  }

  static void * realloc(void * ptr, size_t sz) {
	  return dlrealloc(ptr, sz);
  }
};

#endif
