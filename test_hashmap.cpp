#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include "hashmap.h"

inline unsigned MUTEX_ENTER(unsigned volatile* x) {
  return __sync_lock_test_and_set(x, 0xFF);
}

inline void MUTEX_EXIT(unsigned volatile* x) {
  return __sync_lock_release (x);
}

  class TASLock {
  public:
    volatile unsigned int  _lock;

    TASLock() : _lock(0) {}
    ~TASLock() {_lock=0;}

    inline void init() {_lock = 0;}

    inline void lock() {
      do {
        if (0 == MUTEX_ENTER(&_lock)) {
          return;
        }
      } while(true);
    }

    inline bool tryLock() {
      return ( 0 == MUTEX_ENTER(&_lock) );
    }
    inline bool isLocked() {
      return 0 != _lock;
    }

    inline void unlock() {
      MUTEX_EXIT(&_lock);
    }
  };

  class Memory {
  public:
    inline static void* allocate(const size_t size) {return malloc(size);}
    inline static void  deallocate(void* mem)         {free(mem);}
  };

class HASH_INT {
public:
  inline static size_t Calc(int key, size_t size) {
    key ^= (key << 15) ^ 0xcd7dcd7d;
    key ^= (key >> 10);
    key ^= (key <<  3);
    key ^= (key >>  6);
    key ^= (key <<  2) + (key << 14);
    key ^= (key >> 16);
    return key;
  }

  inline static bool Cmp(int key1, int key2, size_t size) {
    return key1 == key2;
  }

};

class TestHash {
public:
  HashMap<int, int, TASLock, Memory> _ds;
  TestHash() {
  _ds.initialize(HASH_INT::Calc, HASH_INT::Cmp, 1024 );
  }
  
  int get(int key) {
    // We only support lookup
    int value = 0xFFFFFFFF;
    
    if(_ds.find(key, sizeof(int), &value)) {
      fprintf(stderr, "the key %d existing with value %d\n", key, value);
    }
    
    return value;
  }
    
  int put(int key, int value) {
    return _ds.insertIfAbsent(key, sizeof(int), value);
  }

  int remove(int key) {
    return _ds.erase(key, sizeof(int));
  }
  const char* name() {
    return "Chained_MTM";
  }
  void print() {}
  void shutdown() {}
};

int main() {
  HashMap<int, int, TASLock, Memory> hash;

  hash.initialize(HASH_INT::Calc, HASH_INT::Cmp, 1024 );
  hash.insertIfAbsent(5, sizeof(int), 10);
//  fprintf(stderr, "put key 5 to the hashmap\n");

  // Now we are trying to traverse this hashmap.
  HashMap<int, int, TASLock, Memory>::iterator i;

  fprintf(stderr, "First traverse\n");
  for(i = hash.begin(); i != hash.end(); i++) {
    fprintf(stderr, "traverse i %d\n", i.getData());
  }
//  hash.remove(5);
  hash.insertIfAbsent(6, sizeof(int), 11);
  hash.insertIfAbsent(7, sizeof(int), 12);
  //hash.put(6, 11);
  fprintf(stderr, "Second traverse\n");
  for(i = hash.begin(); i != hash.end(); i++) {
    fprintf(stderr, "traverse i %d\n", i.getData());
  }
}

