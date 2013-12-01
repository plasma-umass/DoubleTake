/*
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 1999 - 2008
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#ifndef LIBHX_INTERNAL_H
#define LIBHX_INTERNAL_H 1

#include "config.h"
#include <libHX/defs.h>
#include <libHX/string.h>

#ifdef __cplusplus
	/* Only for our dual C/C++ testsuites */
#	define const_cast(type, expr)       const_cast<type>(expr)
#	define const_cast1(type, expr)      const_cast<type>(expr)
#	define const_cast2(type, expr)      const_cast<type>(expr)
#	define const_cast3(type, expr)      const_cast<type>(expr)
#	define dynamic_cast(type, expr)     dynamic_cast<type>(expr)
#	define signed_cast(type, expr)      signed_cast<type>(expr)
#	define static_cast(type, expr)      static_cast<type>(expr)
#	define reinterpret_cast(type, expr) reinterpret_cast<type>(expr)
#endif

#ifdef __MINGW32__
#	include "uxcompat.h"
#endif
#ifdef _MSC_VER
#	include "uxcompat.h"
#	define snprintf _snprintf
#endif

#ifdef HAVE_VISIBILITY_HIDDEN
#	define EXPORT_SYMBOL __attribute__((visibility("default")))
#else
#	define EXPORT_SYMBOL
#endif

#define MAXFNLEN 256  /* max length for filename buffer */
#define MAXLNLEN 1024 /* max length for usual line */

extern hxmc_t *HXparse_dequote_fmt(const char *, const char *, const char **);

#endif /* LIBHX_INTERNAL_H */
