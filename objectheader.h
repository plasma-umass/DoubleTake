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

#ifndef _OBJECTHEADER_H
#define _OBJECTHEADER_H

/*
 * @file   objectheader.h
 * @brief  Heap object header, including size information and sentinels.
 *         Since all memory blocks are ligned to 8bytes at 32bits machine and 16bytes for 64bits.
 *         We also add some padding here. See 
           http://www.gnu.org/software/libc/manual/html_node/Aligned-Memory-Blocks.html.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

class objectHeader {
public:

  objectHeader (size_t sz)
    : _blockSize (sz)
  {
  }

  size_t getSize () { return (size_t)_blockSize; }

  size_t getObjectSize() { return(size_t)_objectSize; } 

  size_t setObjectSize(size_t sz) {  _objectSize = sz; }

  bool isGoodObject() { 
    return (_sentinel == xdefines::SENTINEL_WORD ? true : false );
  }

  void * getStartPtr() { 
    return ((void *)((intptr_t)&_sentinel + xdefines::SENTINEL_SIZE));
  }

  void setObjectFree() {
    _objectSize = 0;
  }

  bool isObjectFree() {
    return (_objectSize == 0);
  }
 
private:

  // If a block is larger than 4G, we can't support 
  // We are using the lsb of _blockSize bit is marked whether
  // an object is checked or not.
  unsigned int  _blockSize;
  unsigned int  _objectSize;
  
#ifdef X86_32BIT
  int _padding;
#endif
  size_t _sentinel;
};

#endif /* _OBJECTHEADER_H */
