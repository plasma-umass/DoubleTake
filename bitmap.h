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

extern "C" {
  enum { WORDBITS = sizeof(unsigned long) * 8 };
  enum { WORDSIZE = sizeof(unsigned long) * 8 };
  extern unsigned long on[WORDBITS];
  extern unsigned long off[WORDBITS];
};

class bitmap {
public:
	bitmap()
    : _start (NULL),
      _elements (0)
	{ }

  void initialize (void * addr, size_t elements, size_t words) {
    _start = (unsigned long  *)addr;
    _elements = elements;
    _words = words;

    // Initialize the on and off
    for(unsigned long i = 0; i < WORDBITS; i++) {
      on[i] = ((unsigned long)1) << i;
      off[i] = ~on[i];
    }
  }

  inline unsigned long readWord(unsigned long wordIndex) {
    assert(wordIndex <= _words);
    return (_start[wordIndex]);
  }
 
  inline unsigned long * getWord(unsigned long wordIndex) {
    assert(wordIndex <= _words);
    return (&_start[wordIndex]);
  }
 
  inline void getIndexes(unsigned long item, unsigned long *wordIndex, unsigned long *bitIndex) {
    *wordIndex = item/WORDBITS;
    *bitIndex = item%WORDBITS; 
  }

  inline unsigned long getItemIndex(unsigned long wordIndex, unsigned long bitIndex) {
    return wordIndex * WORDBITS + bitIndex;
  }
    
  // Totally, which bit should be set.
  inline void setBit(unsigned long item) {
    assert(item <= _elements);
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    setBit(wordIndex, bitIndex);
  }

  // Inside a word with specifc index, which bits
  inline void setBit(unsigned long wordIndex, int bitIndex) {
    unsigned long * word = getWord(wordIndex);
    *word |= on[bitIndex];
  }

  // Totally, which bit should be set.
  inline bool checkSetBit(unsigned long item) {
    assert(item <= _elements);
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    return checkSetBit(wordIndex, bitIndex);
  }

  // Inside a word with specifc index, which bits
  inline bool checkSetBit(unsigned long wordIndex, int bitIndex) {
    unsigned long * word = getWord(wordIndex);
    bool result = (*word & on[bitIndex] == 0) ? true : false;
    *word |= on[bitIndex];
    return result;
  }

  inline void clearBit(unsigned long item) {
    assert(item <= _elements);
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    clearBit(wordIndex, bitIndex);
  }  

  inline void clearBit(unsigned long wordIndex, int bitIndex) {
    unsigned long * word = getWord(wordIndex);
    *word &= off[bitIndex];
  }

  inline bool isBitSet(unsigned long item) {
    assert(item <= _elements);
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    return isBitSet(wordIndex, bitIndex);
  }

  inline bool isBitSet(unsigned long wordIndex, int bitIndex) {
    unsigned long * word = getWord(wordIndex);
    return ((*word & on[bitIndex]) != 0) ? true : false;
  }

  inline bool hasSetBit(unsigned long bitWord, unsigned long index) {
    return ((bitWord & on[index]) != 0) ? true : false;
  }

  // It is easy to do this if we are using 
  inline bool hasBitSet(unsigned long item, unsigned long totalbits) {
    unsigned long firstWordIndex, firstBitIndex;
    unsigned long lastWordIndex, lastBitIndex;
    getIndexes(item, &firstWordIndex, &firstBitIndex);
    getIndexes(item+totalbits, &lastWordIndex, &lastBitIndex);

    bool hasBit = false;
    unsigned long bitword = readWord(firstWordIndex);

    // check whether all of those bits are inside one word.
    if(firstWordIndex == lastWordIndex) {
      // If they are in the same word. Then we use slower checking.
      // Check bit by bit to find whether there is a bit has been set
      unsigned long bitIndex = firstBitIndex;

      while(bitIndex < lastBitIndex) {
        if(hasSetBit(bitword, bitIndex)) {
          hasBit = true;
          break;
        }
      }
    } else {
      // If not, then it is easy.
      // we should verify whether the first word's lsb index is bitIndex
      if(getLsbIndex(bitword) >= firstBitIndex) {
        hasBit = true;
      } 
      else { 
        // If not, then first word don't have bits set.
        // Then check every word between the first word and last word.
        unsigned long i = firstWordIndex + 1;
        
        while(i < lastWordIndex) {
          bitword = readWord(i);
          if(bitword != 0) {
            hasBit = true;
            break;
          }
        }
        
        if(!hasBit) {
          // If no bit is set, then check last word's msb's index is larger than last bitIndex
          // If yes, then not bit has been set
          bitword = readWord(lastWordIndex);
          if(bitword != 0 && getMsbIndex(bitword) <= lastBitIndex) {
            hasBit = true;
          }    
        }
      }
    }
  }
  inline void clearBits(unsigned long item, unsigned long bits) {
    unsigned long firstWordIndex, firstBitIndex;
    unsigned long lastWordIndex, lastBitIndex;
    getIndexes(item, &firstWordIndex, &firstBitIndex);
    getIndexes(item+bits, &lastWordIndex, &lastBitIndex);

    // Most common case
    if(firstBitIndex == 0 && lastBitIndex == 0) {
      fprintf(stderr, "clearBits: supported cases. firstBitIndex %d lastBitIndex %d lastWordIndex %d\n", firstBitIndex, lastBitIndex, lastWordIndex);
      // Full words.
      void * start = getWord(firstWordIndex);
      int size = (lastWordIndex - firstWordIndex) * WORDSIZE;
      memset(start, 0, size);
    }  
    else {
      fprintf(stderr, "clearBits: none supported cases. firstBitIndex %d lastBitIndex %d\n", firstBitIndex, lastBitIndex);
    }
  }

  inline bool isBitSetOnWord(unsigned long bitword, unsigned long index) {
    return ((bitword & on[index]) != 0) ? true : false;
  }

  // Get last bit has been set
  bool getLastBit(unsigned long item, unsigned long * lastBitIndex) {
    bool hasLastBit = false;
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    // Get the msb in this word. 
    // If bitIndex is the msb, then we have to traverse back 
    // to find last bit in other words. 
    unsigned long bitword = readWord(wordIndex);

    if(bitword != 0) {
      // Should we use lsbIndex or msbIndex?
      unsigned long lsbIndex = getLsbIndex(bitword);
      fprintf(stderr, "lsbIndex is %ld bitIndex %ld\n", lsbIndex, bitIndex);
      //while(1);
      if(bitIndex > lsbIndex) {
        // Then it is possible that there is a bit between lsbIndex (small) and bitIndex (large)
        while(bitIndex > lsbIndex) {
          bitIndex--; 
          if(isBitSetOnWord(bitword, bitIndex)) {
            hasLastBit = true;
            *lastBitIndex = getItemIndex(wordIndex, bitIndex);
            fprintf(stderr, "lastBitIndex is %d at line %d\n", *lastBitIndex, __LINE__);
            break;
          }
        }
      }
    }

    // If we can't find a bit in current bitword, we should check 
    // forward.
    if(!hasLastBit) {
      while(wordIndex > 0) {
        wordIndex--;
        bitword = readWord(wordIndex);
        if(bitword != 0) {
          bitIndex = getMsbIndex(bitword);
          *lastBitIndex = getItemIndex(wordIndex, bitIndex);
          hasLastBit = true;
          break;
        }        
      }
    }

    return hasLastBit;
  }

  unsigned long getMsbIndex(unsigned long word) {
    return WORDBITS -2 -  __builtin_clzl(word);
  }

  unsigned long getLsbIndex(unsigned long word) {
    return __builtin_ffsl(word)-1;
  }

private:
  /// @return a "mask" for the given position.
  inline static unsigned long getMask (int bitIndex) {
    return ((unsigned long) 1) << bitIndex;
  }

  /// The number of bits in a WORD.
  //enum { BYTEBITS = 8 };

  // start address of bitmap.
  unsigned long * _start;

  unsigned long _words;
 
  /// The number of elements in the array.
  unsigned long _elements;

};

#endif
