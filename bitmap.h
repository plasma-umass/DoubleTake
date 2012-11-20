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
 * @file   bitmap.h
 * @brief  The management of global bit map. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 *         Adopted from Diehard project.
 */
#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"
#include "watchpoint.h"

class bitmap {
public:
	bitmap()
    : _bitarray (NULL),
      _elements (0)
	{ }

  // How to calculate the shift bits according to the sector size
  int calcShiftBits(size_t sectorsize) {
    int i = 0;
    
    while((1 << i) < sectorsize) {
      i++;
    }

    if((1 << i) != sectorsize) {
      fprintf(stderr, "Wrong sector size %d, power of 2 is %d\n", sectorsize, 1 >> i);
      abort(); 
    }

    return i;
  }

 
  /**
   * @brief Sets aside space for a certain number of elements.
   * @param  nelts  the number of elements needed.
   */
  void initialize (void * addr, size_t size) {
    assert(_bitarray == NULL);

    // Check whether the size is page aligned.
    assert(size % xdefines::PageSize == 0);

    // Calculate the word shift bits. The word shift bits is used
    // to calculate the word index given an address  
    _wordShiftBits = calcShiftBits(WORDBYTES);
    _itemShiftBits = calcShiftBits(WORDBITS);

    //fprintf(stderr, "WORDBYTES %d and shiftbits %d\n", WORDBYTES, _wordShiftBits);
    // Calculate how many words in the specified size.
    unsigned long nelts = size >> _wordShiftBits;

    // Round up the number of elements to corresponding bits.
    _elements = WORDBITS * ((nelts + WORDBITS - 1) / WORDBITS);

    // Allocate the right number of bytes.
    _totalBytes = getBytes(_elements);
    _heapStart = (intptr_t)addr;
 
    // Now we allocate specific size of shared memory 
    void * buf = MM::mmapAllocateShared(_totalBytes);
    _bitarray = (WORD *) buf;

    // We won't cleanup all bitmap since the actual memory usage can be very small.
    
  //  cleanup();
  }

  inline int getIndex(void * addr) {
    return (((intptr_t)addr - _heapStart) >> _wordShiftBits);
  }

  inline unsigned long getItemFromIndex(unsigned long index) {
    return index >> _itemShiftBits; 
  }
  
  inline int getPositionFromIndex(unsigned long index) {
    return index & (WORDBITS - 1);
  }

  int getBytes(int elements) {
    return (elements/BYTEBITS);
  }

  // Get item and position
  void getItemAndPosition(void * addr, unsigned long * item, unsigned int * pos) {
    unsigned long index = getIndex(addr);
    //fprintf(stderr, "SET bitmap at addr %p, size %d, index %d, getIndex %d\n", addr, (intptr_t)addr - _heapStart, ((intptr_t)addr - _heapStart) >> _wordShiftBits, getIndex(addr));
    assert (index < _elements);
    *item = getItemFromIndex(index); 
    // Position is the position inside one word of bitmap
    *pos = getPositionFromIndex(index);
    //fprintf(stderr, "SET bitmap at addr %p index %d item 0x%lx position 0x%lx\n", addr, index, *item, *pos); 
     return;
  }

  // Calculate corresponding bitmap for given address
  void * getMapPosition(void * heapaddr) {
    unsigned long item;
    unsigned int  pos;

    getItemAndPosition(heapaddr, &item, &pos);

    assert(pos == 0);
    return &_bitarray[item];
  }

  size_t getMapBytes(size_t size) {
    // Calculate how many bytes of bitmap should be cleaned up
    unsigned long nelts = size >> _wordShiftBits;
    size_t bytes = getBytes(nelts);

    return bytes;
  }
 
  /// Clears out the bitmap array when given the start address of heap and size.
  void cleanup(void * start, size_t size) {
    if (_bitarray != NULL) {
      int bytes = getMapBytes(size); 
      void * mapPos = getMapPosition(start);

      // We cleanup the bitmap when a region is allocated.
      memset (mapPos, 0, bytes); // 0 = false
    }
  }

  /// @return true iff the bit was not set (but it is now).
  /// If we are given the address, we have to calculate the "index" at first.
  inline bool tryToSet (void * addr) {
    unsigned long item;
    unsigned int  position;

    getItemAndPosition(addr, &item, &position);

   // fprintf(stderr, "SET bitmap at addr %p item 0x%lx position 0x%lx at bitarray %p\n", addr, item, position, &_bitarray[item]); 
    unsigned long oldvalue;
    const WORD mask = getMask(position);
    oldvalue = _bitarray[item];
    _bitarray[item] |= mask;
    return !(oldvalue & mask);
  }

  /// Clears the bit at the given index.
  /// Return true iff we actually clear this bit.
  inline bool clear (void * addr) {
    unsigned long item;
    unsigned int  position;
    getItemAndPosition(addr, &item, &position);

    unsigned long oldvalue;
    oldvalue = _bitarray[item];
    WORD newvalue = oldvalue &  ~(getMask(position));
    _bitarray[item] = newvalue;
    return (oldvalue != newvalue);
  }

  // Check whether corresponding bit has been set or not. 
  inline bool isSet (void * addr) {
    unsigned long item;
    unsigned int  position;
    getItemAndPosition(addr, &item, &position);
    bool result = _bitarray[item] & (getMask(position));
    return result;
  }

  size_t getWordsOfEachBitmapWord(void) {
    // Each bit of bitmap can be used for one word of original word.
    // For a word with specified bytes, then we can use it for multiple words.
    return ( 1 * WORDBITS); 
  }

  void * getHeapAddress(size_t offset) {
    return ((void *)((intptr_t)_heapStart + offset * BYTEBITS * WORDBYTES));
  }

  // Check whether the sentinels has been corrupted with specified bit map word. 
  inline bool checkIntegrityOnBMW(void * bitmapAddr) {
    size_t bytes;
    bytes = (intptr_t)bitmapAddr - (intptr_t)_bitarray;
  
    WORD * address = (WORD *)getHeapAddress(bytes);
    unsigned long bits = *((WORD *)bitmapAddr);
    //fprintf(stderr, "bitmapAddr %p bytes %d address %p bits %lx\n", bitmapAddr, bytes, address, bits); 
    bool hasCorrupted = false;

    for(int i = 0; i < WORDBITS; i++) {
      // Only check those address when corresponding bit has been set
      if(bits & (getMask(i))) {
        if(address[i] != xdefines::SENTINEL_WORD) {
          hasCorrupted = true;
          watchpoint::getInstance().addWatchpoint(&address[i]);
          fprintf(stderr, "OVERFLOW!!!! now it is 0x%lx at %p\n", address[i], &address[i]);
        }
     //   else {
     //     fprintf(stderr, "Not OVERFLOW, now it is 0x%lx at %p\n", address[i], &address[i]);
     //   }
      }      
    }
     
    return hasCorrupted;
  }

  // Check whether the sentinels of specified range are still integrate or not.
  inline bool checkSentinelsIntegrity(void * addr, size_t size) {
      int bytes = getMapBytes(size); 
      WORD * mapPos = (WORD *)getMapPosition(addr);
      
      int words = bytes / WORDBYTES;
      bool hasCorrupted = false; 

      // We are trying to calculate 
      // We know that for a bit, we can use it for a word.
      // For a word with specified bytes, then we can use it for multiple words.
      for(int i = 0; i < words; i++) {
        if(mapPos[i] != 0) {
          hasCorrupted = checkIntegrityOnBMW((void *)&mapPos[i]);
          if(hasCorrupted) {
            break;
          }
        }
      }

     return hasCorrupted;
  }
 
private:
  typedef size_t WORD;

  /// @return a "mask" for the given position.
  inline static WORD getMask (unsigned long long pos) {
    return ((WORD) 1) << pos;
  }

  /// The number of BYTES in a WORD.
  enum { WORDBYTES = sizeof(WORD) };
 
  /// The number of bits in a WORD.
  enum { WORDBITS = sizeof(WORD) * 8 };

  enum { BYTEBITS = 8 };

  // start address of bitmap.
  WORD * _bitarray;
 
  // Word shift bits is used to calculate the word index given an address.  
  int _wordShiftBits;
  
  // This is used to calculate which item/word of bit map 
  // contains the bit for the word with given index. 
  int _itemShiftBits;

  /// The number of elements in the array.
  unsigned long _elements;
  
  /// How many bytes for the bit map.
  size_t _totalBytes;

  /// Which word should we mark
  unsigned long _heapStart;
};

#endif
