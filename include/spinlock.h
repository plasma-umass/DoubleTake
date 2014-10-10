#if !defined(DOUBLETAKE_SPINLOCK_H)
#define DOUBLETAKE_SPINLOCK_H

/*
 * @file:   spinlock.h
 * @brief:  spinlock used internally.
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 *  Note:   Some references: http://locklessinc.com/articles/locks/   
 */

class spinlock {
public:
  spinlock() {
    _lock = 0;
  }

  void init() {
    _lock = 0;
  }

  // Lock 
  void lock() {
    while(__atomic_exchange_n(&_lock, 1, __ATOMIC_SEQ_CST) == 1) {
      __asm__("pause");
    }
  }
  
  void unlock() {
    __atomic_store_n(&_lock, 0, __ATOMIC_SEQ_CST);
  }

private:
  int _lock;
};

#endif /* __SPINLOCK_H__ */

