/**
 * @file log.cpp
 * @brief Log definitions.
 * @author Bobby Powers
 */

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log.hh"

#define NORMAL_CYAN "\033[36m"
#define NORMAL_MAGENTA "\033[35m"
#define NORMAL_BLUE "\033[34m"
#define NORMAL_YELLOW "\033[33m"
#define NORMAL_GREEN "\033[32m"
#define NORMAL_RED "\033[31m"

#define BRIGHT_CYAN "\033[1m\033[36m"
#define BRIGHT_MAGENTA "\033[1m\033[35m"
#define BRIGHT_BLUE "\033[1m\033[34m"
#define BRIGHT_YELLOW "\033[1m\033[33m"
#define BRIGHT_GREEN "\033[1m\033[32m"
#define BRIGHT_RED "\033[1m\033[31m"

#define ESC_INF NORMAL_CYAN
#define ESC_DBG NORMAL_GREEN
#define ESC_WRN BRIGHT_YELLOW
#define ESC_ERR BRIGHT_RED
#define ESC_END "\033[0m"

#define OUTFD STDERR_FILENO


static const char *LEVEL_NAMES[] = {
  "__bad_level__",
  "INFO",
  "DBG",
  "WRN",
  "ERR",
  "FATALERROR"
};
static const size_t LEVEL_NAMES_LEN = sizeof(LEVEL_NAMES)/sizeof(*LEVEL_NAMES);


// current setting of the global log level - set by default to
// DEBUG_LEVEL, potentially overridden during library initialization
std::atomic_int DT_LOG_LEVEL(DEBUG_LEVEL);

#define FMTBUF_LEN 512


void doubletake::logf(const char *file, int line, int level, const char *fmt, ...) {
  char fmtbuf[FMTBUF_LEN];
  char *tbuf = getCurrentThreadBuffer();

  if (level < 1 || (size_t)level >= LEVEL_NAMES_LEN)
    level = 1;
  const char *lvl_name = LEVEL_NAMES[level];

  ::snprintf(fmtbuf, FMTBUF_LEN,
             ESC_INF "%lx [DOUBLETAKE-%s]: %20s:%-4d: %s" ESC_END "\n",
             pthread_self(), lvl_name, file, line, fmt);

	va_list args;

  va_start(args, fmt);
  ::vsnprintf(tbuf, LOG_SIZE, fmtbuf, args);
  va_end(args);

  write(OUTFD, tbuf, strlen(tbuf));
}

void doubletake::printf(const char *fmt, ...) {
  char fmtbuf[FMTBUF_LEN];
  char *tbuf = getCurrentThreadBuffer();

  ::snprintf(fmtbuf, FMTBUF_LEN-1, BRIGHT_MAGENTA "%s" ESC_END "\n", fmt);

	va_list args;

  va_start(args, fmt);
  ::vsnprintf(tbuf, LOG_SIZE, fmtbuf, args);
  va_end(args);

  write(OUTFD, tbuf, strlen(tbuf));
}

void doubletake::fatalf(const char *file, int line, const char *fmt, ...) {
  va_list args;

  va_start(args, fmt);
  doubletake::logf(file, line, 5, fmt, args);
  va_end(args);

  exit(-1);
}
