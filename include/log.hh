#if !defined(DOUBLETAKE_LOG_H)
#define DOUBLETAKE_LOG_H

/*
 * @file:   log.h
 * @brief:  Logging and debug printing macros
 *          Color codes from SNAPPLE: http://sourceforge.net/projects/snapple/
 * @author: Charlie Curtsinger & Tongping Liu
 */

#include <atomic>

#include "xdefines.hh"

#define LOG_SIZE 4096

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

extern std::atomic_int DT_LOG_LEVEL;

namespace doubletake {
  void logf(const char *file, int line, int level, const char *fmt, ...) __printf_like(4, 5);
  void fatalf(const char *file, int line, const char *fmt, ...) __printf_like(3, 4) __noreturn;
  void printf(const char *fmt, ...) __printf_like(1, 2);
}

/**
 * Print status-information message: level 0
 */
#define _PR(n, ...)                                                                                \
  {                                                                                                \
    if(DT_LOG_LEVEL.load() < n)                                                                    \
      doubletake::logf(__FILE__, __LINE__, n, __VA_ARGS__);                                        \
  }

#ifdef NDEBUG
#define PRINF(...)
#define PRDBG(...)
#define PRWRN(...)
#else
#define PRINF(...) _PR(1, __VA_ARGS__)
#define PRDBG(...) _PR(2, __VA_ARGS__)
#define PRWRN(...) _PR(3, __VA_ARGS__)
#endif /* NDEBUG */

#define PRERR(...) _PR(4, __VA_ARGS__)

#define FATAL(...) doubletake::fatalf(__FILE__, __LINE__, __VA_ARGS__)
#define PRINT(...) doubletake::printf(__VA_ARGS__)

// Check a condition. If false, print an error message and abort
#define REQUIRE(cond, ...)                                                                         \
  if(!(cond))                                                                                      \
    FATAL(__VA_ARGS__)

#endif
