
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
 * @file   hashfuncs.h
 * @brief  Some functions related to hash table.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */
#ifndef __HASHFUNCS_H__
#define __HASHFUNCS_H__

#include "list.h"

class HashFuncs {
public:
  // The following functions are stole from stl library
  static size_t hashString(const void * start, size_t len) {
    unsigned long __h = 0;
    char * __s = (char *)start;
    int i;

    for ( ; i <= len; i++, ++__s)
      __h = 5*__h + *__s;

    return size_t(__h);
  }

  static size_t hashInt(const int x, size_t len) {
    return x;
  }  

  static size_t hashLong(long x, size_t len) {
    return x;
  }

  static size_t hashUnsignedlong(unsigned long x, size_t len) {
    return x;
  }

  static size_t hashAddr(void * addr, size_t len) {
    unsigned long key = (unsigned long)addr;
    key ^= (key << 15) ^ 0xcd7dcd7d;
    key ^= (key >> 10);
    key ^= (key <<  3);
    key ^= (key >>  6);
    key ^= (key <<  2) + (key << 14);
    key ^= (key >> 16);
    return key;
  }

  static bool compareAddr(void * addr1, void * addr2, size_t len) {
    return addr1 == addr2;
  }

  static bool compareInt(int var1, int var2, size_t len) {
    return var1 == var2;
  }

  static bool compareString(const char * str1, const char * str2, size_t len) {
    return strncmp(str1, str2, len) == 0;
  }
  
};

#endif
