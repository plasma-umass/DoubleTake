/*
 *	Shared library handling
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2007 - 2009
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#ifdef _WIN32
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif
#include <libHX/misc.h>
#include "internal.h"

EXPORT_SYMBOL void *HX_dlopen(const char *file)
{
#ifdef _WIN32
	return LoadLibrary(file);
#else
	return dlopen(file, RTLD_LAZY); 
#endif
}

EXPORT_SYMBOL void *HX_dlsym(void *handle, const char *symbol)
{
#ifdef _WIN32
	return GetProcAddress(handle, symbol);
#else
	return dlsym(handle, symbol);
#endif
}

EXPORT_SYMBOL void HX_dlclose(void *handle)
{
#ifdef _WIN32
	FreeLibrary(handle);
#else
	dlclose(handle);
#endif
}

EXPORT_SYMBOL const char *HX_dlerror(void)
{
#ifdef _WIN32
	return "[Error unavailable on WIN32]";
#else
	return dlerror();
#endif
}
