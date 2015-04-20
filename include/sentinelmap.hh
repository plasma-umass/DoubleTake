#if !defined(DOUBLETAKE_SENTINELMAP_H)
#define DOUBLETAKE_SENTINELMAP_H

/*
 * @file   sentinelmap.h
 * @brief  The management of global bit map.
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 *         Adapted from the DieHard project.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <new>

#include "bitmap.hh"
#include "log.hh"
#include "mm.hh"
#include "objectheader.hh"
#include "watchpoint.hh"
#include "xdefines.hh"

class sentinelmap {
public:
  sentinelmap() {}

  // The single instance of sentinelmap. We only need this for
  // heap.
  static sentinelmap& getInstance() {
    static char buf[sizeof(sentinelmap)];
    static sentinelmap* theOneTrueObject = new (buf) sentinelmap();
    return *theOneTrueObject;
  }

  /**
   * @brief Sets aside space for a certain number of elements.
   * @param  nelts  the number of elements needed.
   */
  void initialize(void* addr, size_t size) {
    // Check whether the size is page aligned.
    assert(size % xdefines::PageSize == 0);

    // Calculate the word shift bits. The word shift bits is used
    // to calculate the word index given an address
    // WORDBYTES is 4 for 32bit machine and 8 for 64bit machine
    // so _wordShiftBits is
    _wordShiftBits = calcShiftBits(WORDBYTES);

    // Eachword has 64 bits, so itemShiftBits is 6
    _itemShiftBits = calcShiftBits(WORDBITS);

    //    PRINF("WORDBYTES %d and shiftbits %d _itemShiftBits %d\n", WORDBYTES, _wordShiftBits,
    // _itemShiftBits);
    // Calculate how many words in the specified size.
    _elements = size >> _wordShiftBits;

    // Allocate the right number of bytes.
    _totalBytes = getBytes(_elements);
    _heapStart = (intptr_t)addr;

    // PRINT("**************Sentinelmap INITIALIZATION: elements %lx size %lx. Totalbytes %lx\n",
    // _elements, size, _totalBytes);
    // Now we allocate specific size of shared memory
    void* buf = MM::mmapAllocatePrivate(_totalBytes);
    _bitmap.initialize(buf, _elements, _elements / sizeof(unsigned long));

    // PRINF("bitmap start at buf %p\n", buf);
    // We won't cleanup all bitmap since the actual memory usage can be very small.
    _lastSentinelAddr = NULL;
  }

  /// Clears out the bitmap array when given the start address of heap and size.
  void cleanup(void* start, size_t size) {
    unsigned long item = getIndex(start);
    unsigned long bits = getBitSize(size);

    // PRINF("clearBits item %ld, bits %ld\n", item, bits);
    _bitmap.clearBits(item, bits);
  }

  // Check whether the specified area has some sentinels.
  // Normally, this function will be called before corruption can happen.
  // For example, we call this before irrevocable system calls if
  // this type of system call can not be rolled back safely.
  bool hasSentinels(void* addr, size_t size) {
    // Totally how many words are involved in this.
    unsigned long bits = getBitSize(size);
    unsigned long item = getIndex(addr);

    return _bitmap.hasBitSet(item, bits);
  }

  // Check whether the sentinels of specified range are still integrate or not.
  inline bool checkHeapIntegrity(void* begin, void* end) {
    size_t size = (size_t)((intptr_t)end - (intptr_t)begin);

    // We don't rely on previous sentinel addr if the last address are checked last time
    _lastSentinelAddr = NULL;

    int bytes = getMapBytes(size);
    unsigned long startIndex = getIndex(begin);

    int words = bytes / WORDBYTES;
    bool hasCorrupted = false;

    PRINT("checkSentinelsIntegrity: begin %p end %p bytes %d words %d startindex %ld\n", begin, end, bytes, words, startIndex);
    // We are trying to calculate
    // We know that for a bit, we can use it for a word.
    // For a word with specified bytes, then we can use it for multiple words.
    unsigned long index = startIndex;
    for(auto i = 0; i < (int) words; i++, index++) {
      unsigned long bitword = _bitmap.readWord(index);
      if(bitword != 0) {
				PRINT("checkHeapIntegrity: i %d index %ld\n", i, index);
        if(checkIntegrityOnBMW(bitword, index)) {
          hasCorrupted = true;
        }
      }
    }

    return hasCorrupted;
  }

  /// @return true iff the bit was not set (but it is now).
  /// If we are given the address, we have to calculate the "index" at first.
  inline bool tryToSet(void* addr) {
    unsigned long item = getIndex(addr);
    return _bitmap.checkSetBit(item);
  }

  /// Clears the bit at the given index.
  /// Return true iff we actually clear this bit.
  inline void clear(void* addr) {
    unsigned long item = getIndex(addr);
    _bitmap.clearBit(item);
  }

  // Set up the sentinels for the specified object.
  void setupSentinels(void* ptr, size_t objectsize) {
    size_t* sentinelFirst, *sentinelLast;

    //    PRINF("Setup sentinels ptr %p objectsize %d\n", ptr, objectsize);
    // Calculate the address of two sentinels
    // The organization should be:
    //      objectHeader + sentinelFirst + object (objectsize) + sentinelLast
    sentinelFirst = (size_t*)((intptr_t)ptr - xdefines::SENTINEL_SIZE);
    sentinelLast = (size_t*)((intptr_t)ptr + objectsize);
    *sentinelFirst = xdefines::SENTINEL_WORD;
    *sentinelLast = xdefines::SENTINEL_WORD;

    // PRINF("SET sentinels: first %p (with value %lx) last %p (with value %lx)\n",
    // sentinelFirst,*sentinelFirst, sentinelLast, *sentinelLast);
    // Now we have to set up corresponding bitmap so that we can check heap overflow sometime
    tryToSet((void*)sentinelFirst);
    // PRINF("SET sentinels: setting last %p\n", sentinelLast);
    tryToSet((void*)sentinelLast);
  }

  void setSentinelAt(void* ptr) {
    size_t* sentinel = (size_t*)ptr;

    // Calculate the address of two sentinels
    // The organization should be:
    //      objectHeader + sentinelFirst + object (objectsize) + sentinelLast
    *sentinel = xdefines::SENTINEL_WORD;
    tryToSet(ptr);
    ;
  }

  void clearSentinelAt(void* ptr) { clear(ptr); }

  void setMemalignSentinelAt(void* ptr) {
    size_t* sentinel = (size_t*)ptr;

    // Calculate the address of two sentinels
    // The organization should be:
    //      objectHeader + sentinelFirst + object (objectsize) + sentinelLast
    *sentinel = xdefines::MEMALIGN_SENTINEL_WORD;
    tryToSet((void*)sentinel);
  }

  void markSentinelAt(void* ptr) { tryToSet(ptr); }

  bool checkAndClearSentinel(void* ptr) {
    size_t* sentinel = (size_t*)ptr;
    clear(ptr);

    return ((*sentinel == xdefines::SENTINEL_WORD) ? true : false);
  }

  // Finding the heap object covering the specified address.
  // Now suppose current address is in the middle of an object,
  // then what should we do is to find the last set bit in the bitmap.
  inline bool findObjectStartAddr(void* addr, unsigned long* objectStart) {
    bool hasValidObject = false;

    unsigned long item = getIndex(addr);
    // PRINF("addr %p item %ld\n", addr, item);
    unsigned long* canaryAddr;
    unsigned long startIndex;
    while(_bitmap.getLastBit(item, &startIndex)) {
      // When we get the last set bit in the bitmap, we should check whether
      // this address is inside a valid object.
      // There are at least two cases for a normal object.
      canaryAddr = (unsigned long*)getHeapAddressFromItem(startIndex);
      if(*canaryAddr == xdefines::SENTINEL_WORD) {
        *objectStart = (intptr_t)canaryAddr + xdefines::SENTINEL_SIZE;
        hasValidObject = true;
        break;
      } else if(*canaryAddr == xdefines::MEMALIGN_SENTINEL_WORD) {
        item = startIndex;
        continue;
      } else {
        hasValidObject = false;
        break;
      }
    }

    return hasValidObject;
  }

  // Check whether corresponding bit has been set or not.
  inline bool isSet(void* addr) {
    unsigned long item = getIndex(addr);
    return _bitmap.isBitSet(item);
  }

  inline bool isOverflow(void* addr, void* /* objectStart */, size_t /* objectSize */) { // FIXME
    bool isOverflow = false;

    // Check whether corresponding bit is set.
    // If not set, it is not an overflow.
    if(hasSentinels(addr, WORDBYTES)) {
      WORD curword = *((WORD*)addr);

      // If this bit is set, then we check whether it is a integral or not.
      if(curword != xdefines::SENTINEL_WORD && curword != xdefines::MEMALIGN_SENTINEL_WORD) {
        isOverflow = isCorruptedSentinel(addr);
      }
    }

    return isOverflow;
  }

private:
  // How to calculate the shift bits according to the sector size
  int calcShiftBits(size_t sectorsize) {
    int i = 0;

    while((1UL << i) < sectorsize) {
      i++;
    }

    if((1UL << i) != sectorsize) {
      PRWRN("Wrong sector size %lu, power of 2 is %d\n", sectorsize, 1 >> i);
      abort();
    }

    return i;
  }

  // Calculate word index of an address.
  // Then we can get which bit should we care about.
  inline unsigned long getIndex(void* addr) {
    // printf("getIndex addr %p _heapStart %p wordShiftBits %lx\n", addr, _heapStart,
    // _wordShiftBits);
    return (((intptr_t)addr - _heapStart) >> _wordShiftBits);
  }

  // Calculate the bitmap word from wordIndex
  inline unsigned long getWordIndex(unsigned long index) { return index >> _itemShiftBits; }

  int getBytes(int elements) { return (elements / BYTEBITS); }

  inline unsigned long getBitSize(size_t size) { return size >> _wordShiftBits; }

  size_t getMapBytes(size_t size) {
    // Calculate how many bytes of bitmap should be cleaned up
    unsigned long nelts = size >> _wordShiftBits;

    // Looks like that this getBytes has a bug.
    // But maybe we don't care about this now.

    size_t bytes = getBytes(nelts);
    assert(nelts % 8 == 0);

    return bytes;
  }

  inline bool isBitSet(unsigned long word, int index) {
    // PRINF("isBitSet word %lx index %d getMask(Index) %lx\n", word, index, getMask(index));
    return (((word & getMask(index)) != 0) ? true : false);
  }

  // From the bit map index
  void* getHeapAddressFromWordIndex(size_t index) {
    // Each bit is related to a word (with WORDBYTES)
    // The indexTH's word of bitmap is related to (index * WORDBITS)
    return ((void*)((intptr_t)_heapStart + index * WORDBITS * WORDBYTES));
  }

  void* getHeapAddressFromItem(size_t index) {
    return ((void*)((intptr_t)_heapStart + index * WORDBYTES));
  }

  bool getLastBitIndex(void* bitsAddr, int curIndex, int* index) {
    bool isFound = false;
    WORD bits = *((WORD*)bitsAddr);

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

  inline bool isCorruptedSentinel(void* addr) {
    bool isCorrupted = false;

    // Calculate how many canary bytes here.
    // We are using the ptr to varify the size
    unsigned char* p = (unsigned char*)addr;
    int j = xdefines::WORD_SIZE - 1;
    int magicBytesSize = 0;

    while(p[j] == xdefines::MAGIC_BYTE_NOT_ALIGNED) {
      magicBytesSize++;
      j--;
    }

    // PRWRN(" __CHECK__ magicBytesSize is %d value %lx\n", magicBytesSize, address[i]);
    // If there is no magic bytes, it is wrong since
    // we should have MAGIC_BYTE_NOT_ALIGNED if a bit is set.
    if(magicBytesSize == 0) {
      isCorrupted = true;
    } else if((int)p[j] != magicBytesSize && magicBytesSize != 1) {
      // However, we should be careful on this. Since there is one extreme case,
      isCorrupted = true;
    }
    return isCorrupted;
  }

  // Check whether the sentinels has been corrupted with specified bit map word.
  inline bool checkIntegrityOnBMW(unsigned long bits, unsigned long wordIndex) {
    WORD* address = (WORD*)getHeapAddressFromWordIndex(wordIndex);
    bool checkNonAligned = false;
    bool hasCorrupted = false;

    PRINT("checkSentinelOnBMD: word %ld, bitword %lx address %p\n", wordIndex, bits, address);
    // PRINF("checkSentinelOnBMD: at %lx with value %lx\n", 0x100000028, *((unsigned long
    // *)0x100000028));

    for(int i = 0; i < WORDBITS; i++) {
      // Only check those address when corresponding bit has been set
      if(isBitSet(bits, i)) {
        if(address[i] != xdefines::SENTINEL_WORD &&
           address[i] != xdefines::MEMALIGN_SENTINEL_WORD) {
          bool isBadSentinel = false;
          // Whether this word is filled by MAGIC_BYTE_NOT_ALIGNED
          // If it is true, then next word should be sentinel too.
          if(((i + 1) < WORDBITS) && isBitSet(bits, i + 1)) {
            checkNonAligned = true;
          } else if((i + 1) == WORDBITS) {
            unsigned long nextBits = _bitmap.readWord(wordIndex + 1);
            checkNonAligned = isBitSet(nextBits, 0);
          }

          // this word can be a non-aligned sentinel (partly)
          // if next word is a normal sentinel
          if(checkNonAligned) {
            isBadSentinel = isCorruptedSentinel(&address[i]);
          } else {
            // If aligned, it should be one of preset sentinel
            isBadSentinel = true;
          }

          if(isBadSentinel) {
            PRINT("OVERFLOW!!!! Bit %d at word %lx, aligned %d, now it is 0x%lx at %p\n", i, bits,
                  checkNonAligned, address[i], &address[i]);
            // Find the starting address of this object.
            unsigned long objectStart = 0;

            if(findObjectStartAddr((void*)&address[i], &objectStart)) {
              objectHeader* object = (objectHeader*)(objectStart - sizeof(objectHeader));
            	PRINT("OVERFLOW at line %d object %p address[i] %p\n", __LINE__, object, &address[i]);
              watchpoint::getInstance().addWatchpoint(&address[i], *((size_t*)&address[i]),
                                                      OBJECT_TYPE_OVERFLOW, (void*)objectStart,
                                                      object->getObjectSize());
    					hasCorrupted = true;
            } else {
              // Maybe we can't get the starting address, in this case, we only report when there is
              // a overflow
              continue;
              // watchpoint::getInstance().addWatchpoint(&address[i], *((size_t *)&address[i]),
              // OBJECT_TYPE_OVERFLOW, NULL, 0);
            }
          }
        }
        _lastSentinelAddr = &address[i];
      }
    }
    return hasCorrupted;
  }

  typedef size_t WORD;

  /// @return a "mask" for the given position.
  inline static unsigned long getMask(unsigned long bitIndex) {
    // PRINF("getMask at %d on %lx\n", bitIndex, on[bitIndex]);
    return (1UL << bitIndex);
  }

  /// The number of BYTES in a WORD.
  enum { WORDBYTES = sizeof(WORD) };

  /// The number of bits in a WORD.
  enum { WORDBITS = sizeof(WORD) * 8 };

  enum { BYTEBITS = 8 };

  // start address of bitmap.
  bitmap _bitmap;

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
  void* _lastSentinelAddr;
};

#endif
