#if !defined(DOUBLETAKE_QUARANTINE_H)
#define DOUBLETAKE_QUARANTINE_H

/*
* @file quarantine.h
* @brief Manage those quarantine objects.
         Those objects are freed in a FIFO order when 
         the slots is not enough to hold objects or total size is too large.
         Whichever comes first, we will evict one object. 
* @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
*/

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "watchpoint.hh"
#include "xdefines.hh"

inline size_t getMarkWords(size_t size) {
  int words;

  assert(size % sizeof(unsigned long) == 0);

  // Get a size for checking
  if(size > xdefines::FREE_OBJECT_CANARY_SIZE) {
    words = xdefines::FREE_OBJECT_CANARY_WORDS;
  }
  else {
    words = size/sizeof(unsigned long);
  }
  return words;
}

inline void markFreeObject(void * ptr, size_t size) {
  unsigned long * addr = (unsigned long *)ptr;
  size_t words = getMarkWords(size);

  for(int i = 0; i < words; i++) {
    addr[i] = xdefines::SENTINEL_WORD;  
  }
}

inline bool hasUsageAfterFree(freeObject *object) {
//void * ptr, size_t size) {
  bool hasUAF = false;
  int words = getMarkWords(object->size);

  // We only check specified size
  unsigned long * addr = (unsigned long *)object->ptr;

  for(int i = 0; i < words; i++) {
    if(addr[i] != xdefines::SENTINEL_WORD) {
      hasUAF = true;
      printf("Usage-after-free problem at address %p!!!!!!\n", &addr[i]); 
      // install watchpoints on this point.
      watchpoint::getInstance().addWatchpoint(&addr[i], addr[i], OBJECT_TYPE_USEAFTERFREE, object->ptr, object->size); 
    }     
  }

  return hasUAF; 
}

class quarantine {
public:

  void initialize(void * start, size_t size) {
    _availIndex = 0;
    _LRIndex = 0;
    _totalSize = 0;
    _objectsSize = size;

    _objects = (freeObject *) start;
    _objectsBackup = (freeObject *)((intptr_t)start + size); 
    //PRINF("QUARANTINE list initialize _objects at %p******************************\n", _objects);
  }
 
  void backup() {
    _availIndexBackup = _availIndex;
    _LRIndexBackup = _LRIndex;
    _totalSizeBackup = _totalSize;

    memcpy(_objectsBackup, _objects, _objectsSize); 
  }

  void restore() {
    _availIndex = _availIndexBackup;
    _LRIndex = _LRIndexBackup;
    _totalSize = _totalSizeBackup;

    memcpy(_objects, _objectsBackup, _objectsSize); 
  }

  // We will check whether an object is added into free list or not. 
  // If not, then we can actuall freed an object. 

  bool addFreeObject(void * ptr, size_t size) {
    if(size >= xdefines::QUARANTINE_TOTAL_SIZE) {
      return false;
    }
    // Check whether we need to free some objects
    freeObject * object = &_objects[_availIndex];
    object->ptr  = ptr;
    object->size = size;

   // PRINF("ADDDDDDDDDDDDDD free object ptr %p size %d\n", ptr, size);
    // Mark free object
    markFreeObject(ptr, size);

    _totalSize += size;

    while(_totalSize > xdefines::QUARANTINE_TOTAL_SIZE || !hasAvailSlot()) {
      freeLRObject();
    }

    _availIndex = incrIndex(_availIndex);
    return true;
  }

  inline bool hasAvailSlot() {
    return(((_availIndex + 1)%xdefines::QUARANTINE_BUF_SIZE) != _LRIndex);
  }
 
  inline freeObject * getLRObject() {
    return &_objects[_LRIndex];
  }

  inline int incrIndex(int index) {
    return (index+1)%xdefines::QUARANTINE_BUF_SIZE;
  }

  inline void freeLRObject() {
    // Get the least recent object and verify whether 
    // usage-after-free has been detected?
    freeObject * object = getLRObject();

    // No usage-after-free operation?
    if(!hasUsageAfterFree(object)) {
      // Calling actual heap object to free this object.
      realfree(object->ptr);
 
      // Update corresponding size and object
      _totalSize -= object->size; 
      _LRIndex = incrIndex(_LRIndex);
    }
    else {
      // Calling the rollback.
      rollback();
    }
  }

  
  inline freeObject * retrieveLRObject() {
    freeObject * object = NULL;

    // _LRIndex is never equal to _availIndex unless it is empty.
    if(_LRIndex == _availIndex) {
      return object;
    }

    object = &_objects[_LRIndex];

    _LRIndex=incrIndex(_LRIndex);

    return object;
  }

  bool finalUAFCheck() {
    bool hasUAF = false;
    int  UAFErrors = 0; 
    freeObject * object;
    
//    PRINF("FFFFFFFFFFFinal check\n");

    while ((object = retrieveLRObject())) {
      if(hasUsageAfterFree(object)) {
        UAFErrors++;
        hasUAF = true;

        if(UAFErrors == 4) {
          break;
        }
      }
    }

    return hasUAF;
  }  
private:

  void realfree(void *ptr);
  void rollback();

  freeObject *_objects;
  freeObject *_objectsBackup;

  size_t _objectsSize;

  size_t _totalSize;
  size_t _totalSizeBackup;

  // An index which identifies a slot is available 
  size_t _availIndex;
  size_t _availIndexBackup;

  // An index which indicates which object is least recent object. 
  size_t _LRIndex; 
  size_t _LRIndexBackup;
};

#endif
