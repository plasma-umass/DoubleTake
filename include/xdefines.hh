#ifndef DOUBLETAKE_XDEFINES_H
#define DOUBLETAKE_XDEFINES_H

#include <stddef.h>
#include <ucontext.h>

#ifdef X86_32BIT
#define REG_IP REG_EIP
#define REG_SP REG_ESP
#else
#define REG_IP REG_RIP
#define REG_SP REG_RSP
#endif

#include "list.hh"

/*
 * @file   xdefines.h
 * @brief  Global definitions for Sheriff-Detect and Sheriff-Protect.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/*
#if GCC_VERSION >= 40300
#include <tr1/unordered_map>
#define hash_map std::tr1::unordered_map
#else
#include <ext/hash_map>
#define hash_map __gnu_cxx::hash_map
#endif
*/

#define __printf_like(a, b) __attribute__((format(printf, a, b)))
#define __noreturn __attribute__((noreturn))

extern size_t __max_stack_size;
typedef void* threadFunction(void*);
extern int getThreadIndex();
extern char* getCurrentThreadBuffer();
extern void jumpToFunction(ucontext_t* cxt, unsigned long funcaddr);
extern bool addThreadQuarantineList(void* ptr, size_t size);

// A project can have multiple problems.
// So we make them to use different bits.
typedef enum e_faultyObjectType {
  OBJECT_TYPE_NO_ERROR = 0, // No error for this object
  OBJECT_TYPE_OVERFLOW = 1,
  OBJECT_TYPE_USEAFTERFREE = 2,
  OBJECT_TYPE_LEAK = 4,
  OBJECT_TYPE_WATCHONLY = 8
} faultyObjectType;

inline size_t alignup(size_t size, size_t alignto) {
  return ((size + (alignto - 1)) & ~(alignto - 1));
}

inline size_t aligndown(size_t addr, size_t alignto) { return (addr & ~(alignto - 1)); }

struct watchpointsInfo {
  unsigned long count;
  // Current hardware only support 4 watchpoints in total.
  unsigned long wp[4];
};

#define WP_START_OFFSET sizeof(unsigned long)

struct syncEvent {
  list_t list;
  // Which thread is performing synchronization?
  void* thread;
  void* eventlist;
//	void* syncvariable;
  int ret; // used for mutex_lock
};

struct freeObject {
  void* ptr;
  union {
    int owner; // which thread is using this heap.
    size_t size;
  };
};

class xdefines {
public:
  enum { USER_HEAP_SIZE = 1048576UL * 8192 }; // 8G
  //  enum { USER_HEAP_SIZE     = 1048576UL * 1024 }; // 8G
  enum { USER_HEAP_BASE = 0x100000000 }; // 4G
  enum { MAX_USER_SPACE = USER_HEAP_BASE + USER_HEAP_SIZE };
#ifdef X86_32BIT
  enum { INTERNAL_HEAP_BASE = 0xC0000000 };
#else
  enum { INTERNAL_HEAP_BASE = 0x100000000000 };
#endif
  enum { INTERNAL_HEAP_SIZE = 1048576UL * 4096 };
  enum { INTERNAL_HEAP_END = INTERNAL_HEAP_BASE + INTERNAL_HEAP_SIZE };

  enum { QUARANTINE_BUF_SIZE = 1024 };

  // If total free objects is larger than this size, we begin to
  // re-use those objects
  enum { QUARANTINE_TOTAL_SIZE = 1048576 * 16 };

  // 128M so that almost all memory is allocated from the begining.
  enum { USER_HEAP_CHUNK = 1048576 * 4 };
  enum { INTERNAL_HEAP_CHUNK = 1048576 };
  enum { OBJECT_SIZE_BASE = 16 };
  // enum { MAX_OBJECTS_USER_HEAP_CHUNK = USER_HEAP_CHUNK/MINIMUM_OBJECT_SIZE };
  // enum { TOTAL_USER_HEAP_HUNKS = USER_HEAP_SIZE/USER_HEAP_CHUNK };

	enum { WATCHPOINT_TRAP_MAP_SIZE = 528384 };

  // 4 bytes is representing by 1 bit. If bit is 0, it is not a canary word.
  // Otherwise, it is a canary word.
  enum { BIT_SECTOR_SIZE = 32 };

  enum { MAX_WATCHPOINTS = 4 };
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize - 1) };

  // This is a experimental results. When we are using a larger number, rollback may fail.
  // Don't know why, although maximum number of semaphore is close to 128.
  enum { MAX_ALIVE_THREADS = 128 };

	// Start to reap threads when reaplable threas is larer than that.
  //enum { MAX_REAPABLE_THREADS = 8 };
  enum { MAX_REAPABLE_THREADS = (MAX_ALIVE_THREADS - 10) };
  enum { NUM_HEAPS = MAX_ALIVE_THREADS };
  enum { SYNCMAP_SIZE = 4096 };
  enum { THREAD_MAP_SIZE = 1024 };
  enum { MAX_STACK_SIZE = 0xa00000UL };  // 64pages
  enum { TEMP_STACK_SIZE = 0xa00000UL }; // 64 pages
  enum { NUM_GLOBALS = 128 }; // At least, we need app globals, libc globals and libthread globals.
  // enum { MAX_GLOBALS_SIZE = 1048576UL * 10 };
  enum { CACHE_LINE_SIZE = 64 };

  /**
   * Definition of sentinel information.
   */
  enum { WORD_SIZE = sizeof(size_t) };
  enum { WORD_SIZE_MASK = WORD_SIZE - 1 };
  enum { SENTINEL_SIZE = WORD_SIZE };
  enum { MAGIC_BYTE_NOT_ALIGNED = 0x7E };
  enum { FREE_OBJECT_CANARY_WORDS = 16 };
  enum { FREE_OBJECT_CANARY_SIZE = 16 * WORD_SIZE };
  enum { CALLSITE_MAXIMUM_LENGTH = 10 };

  // FIXME: the following definitions are sensitive to
  // glibc version (possibly?)
  enum { FILES_MAP_SIZE = 4096 };
  enum { MEMTRACK_MAP_SIZE = 4096 };
  enum { FOPEN_ALLOC_SIZE = 0x238 };
  enum { DIRS_MAP_SIZE = 1024 };
  enum { DIROPEN_ALLOC_SIZE = 0x8030 };

  //  enum { MAX_RECORD_ENTRIES = 0x1000 };
  enum { MAX_SYSCALL_ENTRIES = 0x100000 };
  enum { MAX_SYNCEVENT_ENTRIES = 0x1000000 };
  enum { MAX_SYNCVARIABLE_ENTRIES = 0x10000 };

#ifdef X86_32BIT
  enum { SENTINEL_WORD = 0xCAFEBABE };
  enum { MEMALIGN_SENTINEL_WORD = 0xDADEBABE };
#else
  enum { SENTINEL_WORD = 0xCAFEBABECAFEBABE };
  enum { MEMALIGN_SENTINEL_WORD = 0xDADEBABEDADEBABE };
#endif
};

#endif
