#if !defined(DOUBLETAKE_OBJECTHEADER_H)
#define DOUBLETAKE_OBJECTHEADER_H

#include <stddef.h>
#include <stdint.h>

#include "xdefines.hh"

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
  objectHeader(size_t sz)
      : _blockSize(sz), _sentinel(xdefines::SENTINEL_WORD)
  {
  }

  size_t getSize() { return (size_t)_blockSize; }

  size_t getObjectSize() { return (size_t)_objectSize; }

  size_t setObjectSize(size_t sz) {
    _objectSize = sz;
    return sz;
  }

  bool isGoodObject() {
#if defined(DETECT_OVERFLOW) || defined(DETECT_MEMORY_LEAKS)
    return (_sentinel == xdefines::SENTINEL_WORD ? true : false);
#else
    return true;
#endif
  }

  void* getStartPtr() { return ((void*)((intptr_t) & _sentinel + xdefines::SENTINEL_SIZE)); }

  void setObjectFree() { _objectSize = 0; }

  bool isObjectFree() { return (_objectSize == 0); }

#define OBJECT_CHECKED_WORD (0x1)
#define OBJECT_CHECKED_WORD_MASK (0xFFFFFFFE)

  void* getNextObject() {
    return ((void*)((intptr_t) & _sentinel + 4 * xdefines::SENTINEL_SIZE + _blockSize));
  }

  // Since _blockSize is always power of 2 in our allocator,
  // thus we are using the least significant bit to mark whether
  // an heap object is reachable or not.
  void markObjectChecked() { _blockSize |= OBJECT_CHECKED_WORD; }

  void cleanObjectChecked() { _blockSize &= OBJECT_CHECKED_WORD_MASK; }

  // Check whether a object is reachable or not.
  // An object is reachable if it is freed.
  bool doCheckObject() { return (isObjectFree() || isObjectChecked()) ? false : true; }

  // Whether we already checked this object or not.
  bool isObjectChecked() { return (_blockSize & OBJECT_CHECKED_WORD) ? true : false; }

  // Check and clean object is reachable or not.
  bool checkLeakageAndClean() {
    // If an object is freed, it is consider to
    // be reachable.
    bool isLeakage = true;
    if(isObjectFree() || isObjectChecked()) {
      isLeakage = false;

      // cleanObjectChecked
      cleanObjectChecked();
    }

    return isLeakage;
  }

  inline bool isValidObjectSize(unsigned long size) {
    bool isValid = false;
    if(size != 0) {
      isValid = ((size & (size - 1)) == 0);
    }
    return isValid;
  }

  inline bool isValidAddr(unsigned long addr) {
    unsigned long objectSize = getObjectSize();

    bool isValid = isValidObjectSize(objectSize);
    if(isValid) {
      unsigned long objectBegin = (unsigned long)getStartPtr();
      unsigned long objectEnd = objectBegin + getObjectSize();

      if(addr < objectBegin || addr >= objectEnd) {
        isValid = false;
      }
    }

    return isValid;
  }
  //#endif

private:
  // If a block is larger than 4G, we can't support
  // We are using the lsb of _blockSize bit is marked whether
  // an object is checked or not.
  unsigned int _blockSize;
  unsigned int _objectSize;

#ifdef X86_32BIT
  int _padding;
#endif
  size_t _sentinel;
};

#endif
