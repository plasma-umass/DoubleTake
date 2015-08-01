#if !defined(DOUBLETAKE_XCONTEXT_H)
#define DOUBLETAKE_XCONTEXT_H

#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

#include "log.hh"
#include "real.hh"
#include "xdefines.hh"

/**
 * @class xcontext
 * @brief User context to support the rollback mechanism.
 *
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 * @author Bobby Powers
 */

class xcontext {
public:
  xcontext() {}

  void setupBackup(void* ptr) { _backup = ptr; }

  void setupStackInfo(void* privateTop, size_t stackSize) {
    _privateTop = privateTop;
    _stackSize = stackSize;
  }

  void saveCurrent();
  void save(ucontext_t *context);
  void rollback();
  void rollbackInHandler(ucontext_t *context);

  void* getStackTop() { return _privateTop; }

private:
  ucontext_t* getContext() { return &_context; }

  void* getPrivateStart() { return _privateStart; }
  void* getPrivateTop() { return _privateTop; }
  size_t getBackupSize() { return _backupSize; }
  size_t getStackSize() { return _stackSize; }
  void* getBackupStart() { return _backup; }

  /// Saved registers, including the IP, SP, general purpose and floating point.
  ucontext_t _context;

  void* _backup; // Where to _backup the thread private information.
  void* _privateStart;
  void* _privateTop;
  size_t _stackSize;
  size_t _backupSize;
};

#endif
