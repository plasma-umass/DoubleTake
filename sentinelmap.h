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
 * @file   sentinelmap.h
 * @brief  The management of global bit map. 
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 *         Adopted from Diehard project.
 */
#ifndef _SENTINELMAP_H_
#define _SENTINELMAP_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"
#include "mm.h"
#include "objectheader.h"
#include "watchpoint.h"
#include "bitmap.h"

class sentinelmap {
public:
	sentinelmap()
	{ }

  /**
   * @brief Sets aside space for a certain number of elements.
   * @param  nelts  the number of elements needed.
   */
  void initialize (void * addr, size_t size) {
    // Check whether the size is page aligned.
    assert(size % xdefines::PageSize == 0);

    // Calculate the word shift bits. The word shift bits is used
    // to calculate the word index given an address 
    // WORDBYTES is 4 for 32bit machine and 8 for 64bit machine
    // so _wordShiftBits is  
    _wordShiftBits = calcShiftBits(WORDBYTES);
    
    // Eachword has 64 bits, so itemShiftBits is 6
    _itemShiftBits = calcShiftBits(WORDBITS);

//    fprintf(stderr, "WORDBYTES %d and shiftbits %d _itemShiftBits %d\n", WORDBYTES, _wordShiftBits, _itemShiftBits);
    // Calculate how many words in the specified size.
    _elements = size >> _wordShiftBits;

  
    // Allocate the right number of bytes.
    _totalBytes = getBytes(_elements);
    _heapStart = (intptr_t)addr;
 
    fprintf(stderr, "INITIALIZATION: elements %lx size %lx. Totalbytes %lx\n", _elements, size, _totalBytes);
    // Now we allocate specific size of shared memory 
    void * buf = MM::mmapAllocatePrivate(_totalBytes);
    _bitmap.initialize(buf, _elements, _elements/sizeof(unsigned long));

    //fprintf(stderr, "bitmap start at buf %p\n", buf);
    // We won't cleanup all bitmap since the actual memory usage can be very small.
    _lastSentinelAddr = NULL; 
  }

  /// Clears out the bitmap array when given the start address of heap and size.
  void cleanup(void * start, size_t size) {
    unsigned long item = getIndex(start);
    unsigned long bits = getBitSize(size);

    _bitmap.clearBits(item, bits); 
  }

  // Check whether the specified area has some sentinels.
  // Normally, this function will be called before corruption can happen.
  // For example, we call this before irrevocable system calls if 
  // this type of system call can not be rolled back safely.
  bool hasSentinels(void * addr, size_t size) {
    bool hasSentinelInside = false;
    // Totally how many words are involved in this.
    unsigned long bits = getBitSize(size);
    unsigned long item = getIndex(addr);

    return _bitmap.hasBitSet(item, bits);
  }

  // Check whether the sentinels of specified range are still integrate or not.
  inline bool checkSentinelsIntegrity(void * addr, void * stop) {
      size_t size = (size_t)((intptr_t)stop - (intptr_t)addr);

      // We don't rely on previous sentinel addr if the last address are checked last time 
      _lastSentinelAddr = NULL; 


      int bytes = getMapBytes(size); 
      unsigned long startIndex = getIndex(addr);

      WORD * mapPos;

      int words = bytes / WORDBYTES;
      bool hasCorrupted = false; 

      fprintf(stderr, "checkSentinelsIntegrity: addr %p stop %p bytes %d words %d startindex %d\n", addr, stop, bytes, words, startIndex);
      // We are trying to calculate 
      // We know that for a bit, we can use it for a word.
      // For a word with specified bytes, then we can use it for multiple words.
      unsigned long index = startIndex;
      for(unsigned long i = 0; i < words; i++, index++) {
        unsigned long bitword = _bitmap.readWord(index);
        if(bitword != 0) {
          if(checkIntegrityOnBMW(bitword, index)) {
            hasCorrupted = true;
          }
        }
      }

     return hasCorrupted;
  }
  
  /// @return true iff the bit was not set (but it is now).
  /// If we are given the address, we have to calculate the "index" at first.
  inline bool tryToSet (void * addr) {
    unsigned long item = getIndex(addr);
//    fprintf(stderr, "SETSETNTINEL at addr %p item %ld value %lx\n", addr, item, *((unsigned long *)addr));
    return _bitmap.checkSetBit(item);
  }

  /// Clears the bit at the given index.
  /// Return true iff we actually clear this bit.
  inline void clear (void * addr) {
    unsigned long item = getIndex(addr);
    _bitmap.clearBit(item);
  }

private:
  // How to calculate the shift bits according to the sector size
  int calcShiftBits(size_t sectorsize) {
    int i = 0;
    
    while((1UL << i) < sectorsize) {
      i++;
    }

    if((1UL << i) != sectorsize) {
      fprintf(stderr, "Wrong sector size %d, power of 2 is %d\n", sectorsize, 1 >> i);
      abort(); 
    }

    return i;
  }

 
  // Calculate word index of an address. 
  // Then we can get which bit should we care about. 
  inline unsigned long getIndex(void * addr) {
//    printf("getIndex addr %p _heapStart %p index %lx\n", addr, _heapStart, index);
    return (((intptr_t)addr - _heapStart) >> _wordShiftBits);
  }

  // Calculate the bitmap word from wordIndex 
  inline unsigned long getWordIndex(unsigned long index) {
    return index >> _itemShiftBits; 
  }
 
  int getBytes(int elements) {
    return (elements/BYTEBITS);
  }

  inline unsigned long getBitSize(size_t size) {
    return size >> _wordShiftBits;
  }

  size_t getMapBytes(size_t size) {
    // Calculate how many bytes of bitmap should be cleaned up
    unsigned long nelts = size >> _wordShiftBits;

    // Looks like that this getBytes has a bug.
    // But maybe we don't care about this now.

    size_t bytes = getBytes(nelts);
    assert(nelts % 8 == 0);

    return bytes;
  }


  // Check whether corresponding bit has been set or not. 
  inline bool isSet (void * addr) {
    unsigned long item = getIndex(addr);
    return _bitmap.isBitSet(item);
  }

  inline bool isBitSet(unsigned long word, int index) {
   // fprintf(stderr, "isBitSet word %lx index %d getMask(Index) %lx\n", word, index, getMask(index));
    return (((word & getMask(index)) != 0) ? true: false); 
  }

  // From the bit map index
  void * getHeapAddress(size_t index) {
    // Each bit is related to a word (with WORDBYTES)
    // The indexTH's word of bitmap is related to (index * WORDBITS) 
    return ((void *)((intptr_t)_heapStart + index * WORDBITS * WORDBYTES ));
  }

  bool getLastBitIndex(void * bitsAddr, int curIndex, int *index) {
    bool isFound = false;
    WORD bits = *((WORD *)bitsAddr);
    
    for(int i = curIndex; i >= 0; i--) {
      // Only check those address when corresponding bit has been set
      if(isBitSet(bits, i)) {
        *index = i;
        isFound = true;
        break;
      }
    }

    return isFound;

  }

  // Check whether the sentinels has been corrupted with specified bit map word. 
  inline bool checkIntegrityOnBMW(unsigned long bits, unsigned long wordIndex) {
    WORD * address = (WORD *)getHeapAddress(wordIndex);
    bool checkNonAligned = false;
    bool hasCorrupted = false;
          
    //fprintf(stderr, "checkSentinelOnBMD: word %d, bitword %lx address %lx\n", wordIndex, bits, address);
    //fprintf(stderr, "checkSentinelOnBMD: at %lx with value %lx\n", 0x100000028, *((unsigned long *)0x100000028));

    for(int i = 0; i < WORDBITS; i++) {
      // Only check those address when corresponding bit has been set
      if(isBitSet(bits, i)) {
        if(address[i] != xdefines::SENTINEL_WORD && address[i] != xdefines::MEMALIGN_SENTINEL_WORD) {
         //fprintf(stderr, "Bits %ld is set, address %lx with value %lx\n", i, &address[i], address[i]);
          bool isBadSentinel = false;
          // Whether this word is filled by MAGIC_BYTE_NOT_ALIGNED
          // If it is true, then next word should be sentinel too.
          if(((i+1) < WORDBITS) && isBitSet(bits, i+1)) {
            checkNonAligned = true;
          }
          else if((i+1) == WORDBITS) {
            unsigned long nextBits = _bitmap.readWord(wordIndex+1);
            checkNonAligned = isBitSet(nextBits, 0);
          }
          
          // this word can be a non-aligned sentinel (partly) 
          // if next word is a normal sentinel
          if(checkNonAligned) {
            // Calculate how many canary bytes there from the end of this object
            WORD curword = address[i];
            
            // Calculate how many canary bytes here.
            // We are using the ptr to varify the size
            unsigned char * p = (unsigned char *)&address[i];
            int j = xdefines::WORD_SIZE-1;
            int magicBytesSize = 0;

            while(p[j] == xdefines::MAGIC_BYTE_NOT_ALIGNED) {
              magicBytesSize++;
              j--;
            } 

            //fprintf(stderr, " __CHECK__ magicBytesSize is %d value %lx\n", magicBytesSize, address[i]);
            // If there is no magic bytes, it is wrong since 
            // we should have MAGIC_BYTE_NOT_ALIGNED if a bit is set.
            if(magicBytesSize == 0 || (int)p[j] != magicBytesSize) {
              isBadSentinel = true;
            }
          }  // if(checkNonAligned) 
          else { // if(checkNonAligned)
            isBadSentinel = true;
          }
          if(isBadSentinel) {
            fprintf(stderr, "OVERFLOW!!!! now it is 0x%lx at %p\n", address[i], &address[i]);
            watchpoint::getInstance().addWatchpoint(&address[i], *((size_t *)&address[i]));
          }
        }
        _lastSentinelAddr = &address[i];
      }      
    }
    return hasCorrupted;
  }

  typedef size_t WORD;

  /// @return a "mask" for the given position.
  inline static unsigned long getMask (unsigned long bitIndex) {
    //fprintf(stderr, "getMask at %d on %lx\n", bitIndex, on[bitIndex]);
    return (on[bitIndex]);
  }

  /// The number of BYTES in a WORD.
  enum { WORDBYTES = sizeof(WORD) };
 
  /// The number of bits in a WORD.
  enum { WORDBITS = sizeof(WORD) * 8 };

  enum { BYTEBITS = 8 };

  // start address of bitmap.
  bitmap _bitmap;

  bool   _isObjectStart; 
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
  
  // Save last sentinel address for non-aligned overflow detection
  void * _lastSentinelAddr;
};

#endif
