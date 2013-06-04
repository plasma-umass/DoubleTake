// -*- C++ -*-

/*
  Copyright (c) 2012-2013, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file:   log.h
 * @brief:  One file to control different levels of printing.
 *          Code is borrowed from SNAPPLE: http://sourceforge.net/projects/snapple/ 
 * @author: Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "xdefines.h"

extern "C" {
#if 1
#define NORMAL_CYAN "\033[36m"
#define NORMAL_MAGENTA "\033[35m"
#define NORMAL_BLUE "\033[34m"
#define NORMAL_YELLOW "\033[33m"
#define NORMAL_GREEN "\033[32m"
#define NORMAL_RED "\033[31m"

#define BRIGHT "\033[1m"
#define NORMAL "\033[0m"

#define BRIGHT_CYAN "\033[1m\033[36m"
#define BRIGHT_MAGENTA "\033[1m\033[35m"
#define BRIGHT_BLUE "\033[1m\033[34m"
#define BRIGHT_YELLOW "\033[1m\033[33m"
#define BRIGHT_GREEN "\033[1m\033[32m"
#define BRIGHT_RED "\033[1m\033[31m"

#define WHITE_ON_RED "\033[41m"
#define WHITE_ON_GREEN "\033[41m"

#define ESC_INF  NORMAL_CYAN
#define ESC_LOG  NORMAL_GREEN
#define ESC_DBG  BRIGHT_CYAN
#define ESC_WRN  NORMAL_RED
#define ESC_ERR  BRIGHT_RED
#define ESC_END  "\033[0m"
#else
#define NORMAL_CYAN ""
#define NORMAL_MAGENTA ""
#define NORMAL_BLUE ""
#define NORMAL_YELLOW ""
#define NORMAL_GREEN ""
#define NORMAL_RED ""

#define BRIGHT ""
#define NORMAL ""

#define BRIGHT_CYAN ""
#define BRIGHT_MAGENTA ""
#define BRIGHT_BLUE ""
#define BRIGHT_YELLOW ""
#define BRIGHT_GREEN ""
#define BRIGHT_RED ""

#define WHITE_ON_RED ""
#define WHITE_ON_GREEN ""

#define ESC_INF  NORMAL_CYAN
#define ESC_LOG  NORMAL_GREEN
#define ESC_DBG  BRIGHT_CYAN
#define ESC_WRN  NORMAL_RED
#define ESC_ERR  BRIGHT_RED
#define ESC_END  ""
#endif
extern int outfd;

//#define OUTPUT 
#define LOG_SIZE 4096
#define OUTPUT write
/**
 * Print status-information message: level 0
 */
#define PRINF(fmt, ...) \
  {	if(DEBUG_LEVEL < 1) { \
      ::snprintf(getThreadBuffer(), LOG_SIZE, ESC_INF "%lx [PROTO-INFO]: %20s:%-4d: " fmt ESC_END "\n", \
                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ );  \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer()));  } }


/**
 * Print debug message: level 1
 */
#define PRDBG(fmt, ...) \
  {	if(DEBUG_LEVEL < 2) { \
	    ::snprintf(getThreadBuffer(), LOG_SIZE, ESC_DBG "%lx [PROTO-DBG]: %20s:%-4d: " fmt ESC_END "\n",  \
	              pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ );  \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer())); } }

/**
 * Print log message: level 1
 */
#define PRLOG(fmt, ...) \
  {	if(DEBUG_LEVEL < 2) { \
	    ::snprintf(getThreadBuffer(), LOG_SIZE, ESC_LOG "%lx [PROTO-LOG]: %20s:%-4d: " fmt ESC_END "\n", \
                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ );  \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer())); } }


/**
 * Print warning message: level 2
 */
#define PRWRN(fmt, ...) \
  {	if(DEBUG_LEVEL < 3) { \
	    ::snprintf(getThreadBuffer(), LOG_SIZE, ESC_WRN "%lx [PROTO-WARNING]: %20s:%-4d: " fmt ESC_END "\n", \
                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ );  \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer())); } }

/**
 * Print error message: level 3
 */
#define PRERR(fmt, ...) \
  {	if(DEBUG_LEVEL < 4) { \
	    ::snprintf(getThreadBuffer(), LOG_SIZE, ESC_ERR "%lx [PROTO-ERROR]: %20s:%-4d: " fmt ESC_END "\n", \
                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ );  \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer())); } }


/**
 * Print fatal error message, the program is going to exit.
 */
#define PRFATAL(fmt, ...) \
  {	  ::snprintf(getThreadBuffer(), LOG_SIZE, ESC_ERR "%lx [PROTO-FATALERROR]: %20s:%-4d: " fmt ESC_END "\n", \
                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ ); exit(-1); \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer()));  }
//                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ ); exit(-1); \
                pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__ ); \
      OUTPUT(outfd, getThreadBuffer(), strlen(getThreadBuffer()));  }

};

#endif /* _ */
