/*
 * @file:   log.h
 * @brief:  Logging and debug printing macros
 *          Color codes from SNAPPLE: http://sourceforge.net/projects/snapple/ 
 * @author: Charlie Curtsinger & Tongping Liu
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <assert.h>

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

#define ESC_LOG  NORMAL_GREEN
#define ESC_WRN  BRIGHT_YELLOW
#define ESC_ERR  BRIGHT_RED
#define ESC_END  "\033[0m"

// Print a warning message, regardless of debug build level
#define WARN(fmt, ...)  fprintf(stderr, ESC_WRN " [%s:%d] Warning: " fmt ESC_END "\n", __FILE__, __LINE__, ##__VA_ARGS__)

// Print an error message, regardless of debug build level
#define ERROR(fmt, ...) fprintf(stderr, ESC_ERR " [%s:%d] Error: " fmt ESC_END "\n", __FILE__, __LINE__, ##__VA_ARGS__)

// Check a condition. If false, print an error message and abort
#define REQUIRE(cond, ...) if(!(cond)) { ERROR(__VA_ARGS__); abort(); }

#if !defined(NDEBUG)
// Print a debug message
# define DEBUG(fmt, ...) fprintf(stderr, ESC_LOG " [%s:%d] " fmt ESC_END "\n", __FILE__, __LINE__, ##__VA_ARGS__)
// Check a condition and abort if false
# define PREFER(cond, ...) if(!(cond)) { WARN(__VA_ARGS__); abort(); }
#else
// Hide debug message in release mode
# define DEBUG(...)
// Check a condition and warn if false
# define PREFER(cond, ...) if(!(cond)) { WARN(__VA_ARGS__); abort(); }
#endif

#endif
