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

#include <assert.h>

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


/**
 * Print status-information message: level 0
 */
#define PRINF(fmt, ...) \
  {	if(DEBUG_LEVEL < 1) \
      ::fprintf(stderr, ESC_INF "%d [PROTO]: %20s:%-4d: " fmt ESC_END "\n", \
                getpid(), __FILE__, __LINE__, ##__VA_ARGS__ ); }

/**
 * Print log message: level 1
 */
#define PRLOG(fmt, ...) \
  {	if(DEBUG_LEVEL < 2) \
	    ::fprintf(stderr, ESC_LOG "%d [PROTO]: %20s:%-4d: " fmt ESC_END "\n", \
                getpid(), __FILE__, __LINE__, ##__VA_ARGS__ ); }

/**
 * Print debug message: level 2
 */
#define PRDBG(fmt, ...) \
  {	if(DEBUG_LEVEL < 3) \
	    ::fprintf(stderr, ESC_DBG "%d [PROTO-DBG]: %20s:%-4d: " fmt ESC_END "\n",  \
	              getpid(), __FILE__, __LINE__, ##__VA_ARGS__ ); }



/**
 * Print warning message: level 3
 */
#define PRWRN(fmt, ...) \
  {	if(DEBUG_LEVEL < 4) \
	    ::fprintf(stderr, ESC_WRN "%d [PROTO-WARNING]: %20s:%-4d: " fmt ESC_END "\n", \
                getpid(), __FILE__, __LINE__, ##__VA_ARGS__ ); }

/**
 * Print error message: level 4
 */
#define PRERR(fmt, ...) \
  {	if(DEBUG_LEVEL < 5) \
	    ::fprintf(stderr, ESC_ERR "%d [PROTO-ERROR]: %20s:%-4d: " fmt ESC_END "\n", \
                getpid(), __FILE__, __LINE__, ##__VA_ARGS__ ); }


/**
 * Print fatal error message, the program is going to exit.
 */
#define PRFATAL(fmt, ...) \
  {	if(DEBUG_LEVEL < 5) \
	    ::fprintf(stderr, ESC_ERR "%d [PROTO-ERROR]: %20s:%-4d: " fmt ESC_END "\n", \
                getpid(), __FILE__, __LINE__, ##__VA_ARGS__ ); exit(-1); }

#endif /* _ */
