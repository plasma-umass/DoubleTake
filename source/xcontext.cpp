/**
 * @class xcontext
 * @brief User context to support the rollback mechanism.
 *
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include "xcontext.hh"

// these functions are defined in assembly so that they can safely
// swap the stack underneath themselves
extern "C" {
  void __dt_rollback_ctx(void *dst, void *src, size_t len, ucontext_t *ctx);
}

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE-1)
#define PAGE_ALIGN(x) (((intptr_t)(x) + (intptr_t)(PAGE_MASK)) & ~PAGE_MASK)
#define PAGE_ALIGN_DOWN(x) PAGE_ALIGN(x) - PAGE_SIZE

static intptr_t getStackPointer(ucontext* uctx) {
  return uctx->uc_mcontext.gregs[REG_SP];
}

void xcontext::save(ucontext_t *uctx) {
  intptr_t sp;
  intptr_t stackBottom;
  size_t size;

  sp = getStackPointer(uctx);
  stackBottom = PAGE_ALIGN_DOWN(sp);

  _privateStart = (void *)stackBottom;
  size = (intptr_t)_privateTop - stackBottom;
  _backupSize = size;

  REQUIRE(size < _stackSize,
          "Wrong. Current stack size (%zx = %p - %p) is larger than total size (%zx)\n",
          size, _privateTop, _privateStart, _stackSize);

  Real::mprotect(_backup, size, PROT_WRITE);
  memcpy(_backup, _privateStart, size);
  Real::mprotect(_backup, size, PROT_NONE);

  // We are trying to save context at first
  memcpy(&_context, uctx, sizeof(ucontext_t));
}

void xcontext::saveCurrent() {
  intptr_t sp;
  intptr_t stackBottom;
  size_t size;

#if defined(X86_32BIT)
  asm volatile(
    // directly grab esp
    "movl %%esp, %0\n"
    : "=r"(sp));
#else
  asm volatile(
    // directly grab rsp
    "movq %%rsp, %0\n"
    : "=r"(sp));
#endif

  stackBottom = PAGE_ALIGN_DOWN(sp);

  _privateStart = (void *)stackBottom;
  size = (intptr_t)_privateTop - stackBottom;
  _backupSize = size;

  PRINF("Backup size: %zu/%zu", size, _stackSize);

  REQUIRE(size < _stackSize,
          "Stack too large. top:%p sp:%p PAGE_ALIGN(sp):%p size:%zu",
          _privateTop, (void *)sp, (void *)stackBottom, size);

  Real::mprotect(_backup, size, PROT_WRITE);
  memcpy(_backup, _privateStart, size);
  getcontext(&_context);
  Real::mprotect(_backup, size, PROT_NONE);
}

void xcontext::rollback() {
  Real::mprotect(_backup, _backupSize, PROT_READ);
  // call an arch-specific routine to safely copy the stack under us
  // (requires memcpy w/o using the stack, something I don't think we
  // can guarantee in C) and call setcontext
  __dt_rollback_ctx(_privateStart, _backup, _backupSize, &_context);
}

// perform a rollback while in the signal handler - this ucontext_t is
// passed back to the kernel on return from the signal handler and
// execution resumes there.  Additionally, SIGUSR2 is masked while the
// handler is running so we don't have to worry about receiving a
// signal in the middle of this method.
void xcontext::rollbackInHandler(ucontext_t* kctx) {
  memcpy(_privateStart, _backup, _backupSize);
  memcpy(kctx, &_context, sizeof(_context));
}
