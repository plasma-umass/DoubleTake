#if !defined(DOUBLETAKE_BITMAP_H)
#define DOUBLETAKE_BITMAP_H

/*
 * @file   bitmap.h
 * @brief  The management of global bit map.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 *         Adopted from Diehard project.
 */

#include <stdio.h>

#include "xdefines.hh"

class bitmap {
private:
  enum { WORDBITS = sizeof(unsigned long) * 8 };
  enum { WORDSIZE = sizeof(unsigned long) * 8 };

  unsigned long on[WORDBITS];
  unsigned long off[WORDBITS];

public:
  bitmap() : _start(NULL), _words(0), _elements(0) {}

  void initialize(void* addr, size_t elements, size_t words) {
    _start = (unsigned long*)addr;
    _elements = elements;
    _words = words;

    //    PRINF("*********************bitmap initialize %p****************\n", addr);
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

  inline unsigned long* getWord(unsigned long wordIndex) {
    assert(wordIndex <= _words);
    return (&_start[wordIndex]);
  }

  inline void getIndexes(unsigned long item, unsigned long* wordIndex, unsigned long* bitIndex) {
    *wordIndex = item / WORDBITS;
    *bitIndex = item % WORDBITS;
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
    unsigned long* word = getWord(wordIndex);
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
  inline bool checkSetBit(unsigned long wordIndex, unsigned long bitIndex) {
    unsigned long* word = getWord(wordIndex);
    //   PRINF("checkSetBit wordIndex %d bitIndex %d word %lx\n", wordIndex, bitIndex, *word);
    bool result = ((*word & on[bitIndex]) == 0) ? true : false;
    *word |= on[bitIndex];
    //  PRINF("checkSetBit wordIndex %d bitIndex %d word %lx\n", wordIndex, bitIndex, *word);
    return result;
  }

  inline void clearBit(unsigned long item) {
    assert(item <= _elements);
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    clearBit(wordIndex, bitIndex);
  }

  inline void clearBit(unsigned long wordIndex, int bitIndex) {
    unsigned long* word = getWord(wordIndex);
    *word &= off[bitIndex];
  }

  inline bool isBitSet(unsigned long item) {
    assert(item <= _elements);
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    return isBitSet(wordIndex, bitIndex);
  }

  inline bool isBitSet(unsigned long wordIndex, int bitIndex) {
    unsigned long* word = getWord(wordIndex);
    return ((*word & on[bitIndex]) != 0) ? true : false;
  }

  inline bool hasSetBit(unsigned long bitWord, unsigned long index) {
    return ((bitWord & on[index]) != 0) ? true : false;
  }

  // It is easy to do this if we are using
  bool hasBitSet(unsigned long item, unsigned long totalbits) {
    unsigned long firstWordIndex, firstBitIndex;
    unsigned long lastWordIndex, lastBitIndex;
    getIndexes(item, &firstWordIndex, &firstBitIndex);
    getIndexes(item + totalbits, &lastWordIndex, &lastBitIndex);

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
        bitIndex++;
      }

    } else {
      // If not, then it is easy.
      // we should verify whether the first word's lsb index is bitIndex
      if(getLsbIndex(bitword) >= firstBitIndex) {
        hasBit = true;
      } else {
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
    return hasBit; // EDB
  }

  inline void clearBits(unsigned long item, unsigned long bits) {
    unsigned long firstWordIndex, firstBitIndex;
    unsigned long lastWordIndex, lastBitIndex;
    getIndexes(item, &firstWordIndex, &firstBitIndex);
    getIndexes(item + bits, &lastWordIndex, &lastBitIndex);

    // Most common case
    if(firstBitIndex == 0 && lastBitIndex == 0) {
      //   PRINF("clearBits: supported cases. firstBitIndex %d lastBitIndex %d lastWordIndex %d\n",
      // firstBitIndex, lastBitIndex, lastWordIndex);
      // Full words.
      void* start = getWord(firstWordIndex);
      int size = (lastWordIndex - firstWordIndex) * WORDSIZE;
      memset(start, 0, size);
    } else {
      assert(0);
      // PRINF("clearBits: none supported cases. firstBitIndex %d lastBitIndex %d\n", firstBitIndex,
      // lastBitIndex);
    }
  }

  inline bool isBitSetOnWord(unsigned long bitword, unsigned long index) {
    return ((bitword & on[index]) != 0) ? true : false;
  }

  // Get last bit has been set
  bool getLastBit(unsigned long item, unsigned long* lastBitIndex) {
    bool hasLastBit = false;
    unsigned long wordIndex, bitIndex;
    getIndexes(item, &wordIndex, &bitIndex);
    // Get the msb in this word.
    // If bitIndex is the msb, then we have to traverse back
    // to find last bit in other words.
    unsigned long bitword = readWord(wordIndex);
    // PRINT("wordindex %lx bitword %lx bitIndex %ld\n", wordIndex, bitword, bitIndex);

    if(bitword != 0) {
      // Should we use lsbIndex or msbIndex?
      unsigned long lsbIndex = getLsbIndex(bitword);
      if(bitIndex > lsbIndex) {
        // Then it is possible that there is a bit between lsbIndex (small) and bitIndex (large)
        while(bitIndex > lsbIndex) {
          bitIndex--;
          if(isBitSetOnWord(bitword, bitIndex)) {
            hasLastBit = true;
            *lastBitIndex = getItemIndex(wordIndex, bitIndex);
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
          // PRINT("bitword %lx wordIndex %d bitIndex %d\n", bitword, wordIndex, bitIndex);
          *lastBitIndex = getItemIndex(wordIndex, bitIndex + 1);
          hasLastBit = true;
          break;
        }
      }
    }

    return hasLastBit;
  }

  unsigned long getMsbIndex(unsigned long word) { return WORDBITS - 2 - __builtin_clzl(word); }

  unsigned long getLsbIndex(unsigned long word) { return __builtin_ffsl(word) - 1; }

private:
  /// @return a "mask" for the given position.
  inline static unsigned long getMask(int bitIndex) { return ((unsigned long)1) << bitIndex; }

  /// The number of bits in a WORD.
  // enum { BYTEBITS = 8 };

  // start address of bitmap.
  unsigned long* _start;

  unsigned long _words;

  /// The number of elements in the array.
  unsigned long _elements;
};

#endif
