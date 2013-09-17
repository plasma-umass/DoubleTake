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
#include "mm.h"
#include "objectheader.h"
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
    
    while((1UL << i) < sectorsize) {
      i++;
    }

    if((1UL << i) != sectorsize) {
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
    // WORDBYTES is 4 for 32bit machine and 8 for 64bit machine
    // so _wordShiftBits is  
    _wordShiftBits = calcShiftBits(WORDBYTES);
    _itemShiftBits = calcShiftBits(WORDBITS);

    //fprintf(stderr, "WORDBYTES %d and shiftbits %d _itemShiftBits %d\n", WORDBYTES, _wordShiftBits, _itemShiftBits);
    // Calculate how many words in the specified size.
    unsigned long nelts = size >> _wordShiftBits;

    // Round up the number of elements to corresponding bits.
    _elements = WORDBITS * ((nelts + WORDBITS - 1) / WORDBITS);
    //printf("INITIALIZATION: elements %lx %ld size %lx\n", _elements, _elements, size);
  
    // Allocate the right number of bytes.
    _totalBytes = getBytes(_elements);
    _heapStart = (intptr_t)addr;
 
    // Now we allocate specific size of shared memory 
    void * buf = MM::mmapAllocatePrivate(_totalBytes);
    _bitarray = (WORD *) buf;

    //fprintf(stderr, "bitmap start at buf %p\n", buf);
    // We won't cleanup all bitmap since the actual memory usage can be very small.
    _lastSentinelAddr = NULL; 
  }

  inline unsigned long getIndex(void * addr) {
//    printf("getIndex addr %p _heapStart %p index %lx\n", addr, _heapStart, index);

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
    if(index >= _elements) {
      printf("getItemAndPosition, addr %p index %d elements %d\n", addr,getIndex(addr), _elements);
    }
    //fprintf(stderr, "SET bitmap at addr %p, size %d, index %d, getIndex %d, elements %d\n", addr, (intptr_t)addr - _heapStart, ((intptr_t)addr - _heapStart) >> _wordShiftBits, getIndex(addr), _elements);
    assert (index < _elements);
    *item = getItemFromIndex(index); 
    // Position is the position inside one word of bitmap
    *pos = getPositionFromIndex(index);
    //fprintf(stderr, "SET bitmap at addr %p index %d item 0x%lx position 0x%lx\n", addr, index, *item, *pos); 
     return;
  }

  bool isAlignedSize(size_t size) {
    // since one bit is used to indentify one word
    // one bitmap word usually has 64 bit (8 bytes) for , which  
  }

  
  // Check whether the specified area has some sentinels.
  // Normally, this function will be called before the corruption can happen.
  // For example, we call this before system calls if 
  // this type of system call can not be rolled back safely.
  bool hasSentinels(void * addr, size_t size) {
    bool hasSentinelInside = false;
    unsigned long nelts = size >> _wordShiftBits;
    unsigned long item;
    unsigned int  pos;

    getItemAndPosition(addr, &item, &pos);

    WORD * mapPos = (WORD *)(&_bitarray[item]);
    
    unsigned long words = nelts/WORDBITS;
    unsigned long moreBits = nelts % WORDBITS;
    unsigned long firstBitsEnd, lastBitsEnd; // How many bits we should check in the first word. 

    // Check whether there are some sentinels in the first WORD.
    // Since the first WORD can hold (WORDBITS - pos).
    if((WORDBITS - pos) >= moreBits) {
      if(words == 0) {
        firstBitsEnd = pos + moreBits;
        lastBitsEnd = 0;
        words = 0;
      }
      else {
        firstBitsEnd = WORDBITS;
        lastBitsEnd = pos + moreBits;
        words--; // complete words
      }
    }
    else {
      firstBitsEnd = WORDBITS;
      lastBitsEnd = moreBits - (WORDBITS - pos);
    }
    
    {
      WORD bits = mapPos[0];   
      for(int i = pos; i < firstBitsEnd; i++) {
        // Calculate the start address of map
        if(isBitSet(bits, i)) {
          hasSentinelInside = true;
//          fprintf(stderr, "hasSentinelInside when i %d at the first word\n", i);
          break;
        }
      }
    }

    // Check those parts with complete word
    if(hasSentinelInside != true) {
      for(int i = 1; i <= words; i++) {
        if(mapPos[i] != 0) {
          hasSentinelInside = true;
//          fprintf(stderr, "has sentinel inside now with word %d\n", i);
          break;
        }      
      }
    }

    // Now we are checking the last word. 
    // Don't check when we already find out some sentinels there.
    if(lastBitsEnd != 0 && hasSentinelInside != true) {
      // Check the next bitmap word.
      WORD bits = mapPos[words+1];   
     
      if(bits != 0) {
        for(int i = 0; i < lastBitsEnd; i++) {
          if(isBitSet(bits, i)) {
            hasSentinelInside = true;
            //fprintf(stderr, "has sentinel inside at last word, words %d, bit %d lastBitsEnd %d\n", words, i, lastBitsEnd);
            break;
          }
        }
      } 
      // We should check those aligned and un-aligned words. 
    }

    if(hasSentinelInside) {
      fprintf(stderr, "System call are corrupting on %p and size %x\n", addr, size); 
      fprintf(stderr, "nelts %d, words %d pos %d morebits %d firstBitEnd %d lastBitsEnd %d\n", nelts, words, pos, moreBits, firstBitsEnd, lastBitsEnd); 
    }

    return hasSentinelInside;
  }

  // Calculate corresponding bitmap for given address
  void * getMapPosition(void * heapaddr) {
    unsigned long item;
    unsigned int  pos;

    getItemAndPosition(heapaddr, &item, &pos);

   // fprintf(stderr, "heapaddr %p item %lx pos %lx\n", heapaddr, item, pos);
    assert(pos == 0);
    return &_bitarray[item];
  }

  size_t getMapWords(size_t nelts) {
    return nelts / WORDBITS;
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

  inline bool isBitSet(unsigned long bits, int index) {
    return (((bits & getMask(index)) != 0) ? true: false); 
  }

  inline unsigned long getNextBits(void * Addr) {
    WORD * mapPos = (WORD *)Addr; 
    return (unsigned long)(*((WORD *)&mapPos[1]));
  }
  
  inline unsigned long getLastBits(void * Addr) {
    WORD * mapPos = (WORD *)Addr; 
    return (unsigned long)(*((WORD *)&mapPos[-1]));
  }

  void * getHeapAddressFromBits(size_t bits) {
    return ((void *)((intptr_t)_heapStart + bits * WORDBYTES));
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
  // We are trying to check the bitmap to find out where last bit is set
  // since it means the start of current object.
  inline void * getLastBitSetAddr(void * bitmapAddr, int index) {
    bool isFound = false;
    WORD * bitWord;
    int    lastIndex;

    // Check whether a bit is set in current bitmap word
    if(index > 0) {
      isFound = getLastBitIndex(bitmapAddr, index-1, &lastIndex);
    }

    // If not, try to find the last bitmap word which is not 0.
    if(!isFound) {
      // We will start from the bitWord before current bitWord.
      bitWord = (WORD *)bitmapAddr;
      bitWord--;

      while(*bitWord == 0) {
        bitWord--;
      }

      assert(bitWord != 0);

      // Now we find a bitWord is not 0, try to get the last bit from it.
      isFound = getLastBitIndex(bitWord, WORDBITS, &lastIndex);
      assert(isFound == true);
    }

    // Find out the corresponding address with specified bitword and index
    size_t totalBits  = ((intptr_t)bitmapAddr - (intptr_t)_bitarray) * BYTEBITS + lastIndex;
  
    WORD * address = (WORD *)getHeapAddressFromBits(totalBits);

    return address;
  }

  // Check whether the sentinels has been corrupted with specified bit map word. 
  inline bool checkIntegrityOnBMW(void * bitmapAddr) {
    size_t bytes;
    bytes = (intptr_t)bitmapAddr - (intptr_t)_bitarray;
  
    WORD * address = (WORD *)getHeapAddress(bytes);
    unsigned long bits = *((WORD *)bitmapAddr);
    bool checkNonAligned = false;
    bool hasCorrupted = false;

    for(int i = 0; i < WORDBITS; i++) {
      // Only check those address when corresponding bit has been set
      if(isBitSet(bits, i)) {
        if(address[i] != xdefines::SENTINEL_WORD && address[i] != xdefines::MEMALIGN_SENTINEL_WORD) {
          bool isBadSentinel = false;
#ifdef DETECT_NONALIGNED_OVERFLOW
          // Whether this word is filled by MAGIC_BYTE_NOT_ALIGNED
          // If it is true, then next word should be sentinel too.
          if(((i+1) < WORDBITS) && isBitSet(bits, i+1)) {
            checkNonAligned = true;
          }
          else if((i+1) == WORDBITS) {
            unsigned long nextBits = getNextBits(bitmapAddr);
            checkNonAligned = isBitSet(nextBits, 0);
          }
          
          
          // this word can be a non-aligned sentinel (partly) 
          // if next word is a normal sentinel
          if(checkNonAligned) {
          //  assert(_lastSentinelAddr != NULL);
            // The previous word can not be a normal sentinel if next 
            // word is a normal sentinel
//            fprintf(stderr, "before gatLastBitSetAddr, checkNonAligned is %d _lastSentinelAddr %p\n", checkNonAligned, _lastSentinelAddr);
            // Calculate how many canary bytes there from the end of this object
            WORD curword = address[i];
            
            // Calculate how many canary bytes here.
            // We are using the ptr to varify the size
            unsigned char * p = (unsigned char *)&address[i];
            int j = xdefines::WORD_SIZE-1;
            int magicBytesSize = 0;

            // We don't need to check j's length
            //fprintf(stderr, "initially magicBytesSize is %d word %lx at %p first one %lx\n", magicBytesSize, address[i], p, p[j]);
            while(p[j] == xdefines::MAGIC_BYTE_NOT_ALIGNED) {
              magicBytesSize++;
              j--;
            } 

            if((int)p[j] != magicBytesSize) {
              //fprintf(stderr, "magicBytesSize is %d overflow %lx\n", magicBytesSize, address[i]);
              isBadSentinel = true;
            }
          }  // if(checkNonAligned) 
          else { // if(checkNonAligned)
            isBadSentinel = true;
          }
#else 
          isBadSentinel = true;
#endif
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

  // Check whether the sentinels of specified range are still integrate or not.
  inline bool checkSentinelsIntegrity(void * addr, void * stop) {
      size_t size = (size_t)((intptr_t)stop - (intptr_t)addr);

      //fprintf(stderr, "checkSentinelsIntegrity addr %p stop %p with size %x\n", addr, stop,  size);
      // We don't rely on previous sentinel addr if the last address are checked last time 
      _lastSentinelAddr = NULL; 

      int bytes = getMapBytes(size); 
      WORD * mapPos = (WORD *)getMapPosition(addr);
      
      int words = bytes / WORDBYTES;
    //  fprintf(stderr, "checkSentinelsIntegrity start %p stop %p size %x bytes %d words %d\n", addr, stop,size, bytes, words);
      bool hasCorrupted = false; 

      // We are trying to calculate 
      // We know that for a bit, we can use it for a word.
      // For a word with specified bytes, then we can use it for multiple words.
      for(int i = 0; i < words; i++) {
        if(mapPos[i] != 0) {
          if(checkIntegrityOnBMW((void *)&mapPos[i])) {
            hasCorrupted = true;
          }
        }
      }

     return hasCorrupted;
  }
 
private:
  typedef size_t WORD;

  inline objectHeader * getObjectHeader(unsigned long lastAddr) {
    objectHeader * o = (objectHeader *)(lastAddr - sizeof(objectHeader) + xdefines::SENTINEL_SIZE);

    // Check whether place of last addr is a valid sentinel
    if(!o->isGoodObject()) {
//      assert(0);
      return NULL;
    }
    else {
      return o;
    }  
  }

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
  
  // Save last sentinel address for non-aligned overflow detection
  void * _lastSentinelAddr;
};

#endif
