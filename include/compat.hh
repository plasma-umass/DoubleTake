#ifndef DOUBLETAKE_COMPAT_H
#define DOUBLETAKE_COMPAT_H

// including the C++ 'cstddef' header, rather than 'stddef.h', causes
// issues for Clang < 3.6.  We only include stddef.h in DoubleTake,
// but HeapLayers uses cstddef.  If support for Clang < 3.6 is ever
// removed, this file can be deleted.  The definition is from upstream
// clang: http://reviews.llvm.org/rL201729

#if defined(__clang__)  && __clang_major__ <= 3 && __clang_minor__ <= 5
typedef struct {
  long long __clang_max_align_nonce1
  __attribute__((__aligned__(__alignof__(long long))));
  long double __clang_max_align_nonce2
  __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;
#define __CLANG_MAX_ALIGN_T_DEFINED
#endif

#endif // DOUBLETAKE_COMPAT_H
