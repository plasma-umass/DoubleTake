/*
 *	libHX/ux-mmap.c
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2005 - 2006
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <sys/types.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include "internal.h"
#include <libHX/misc.h>

static inline DWORD dw_desired_access(int, int);
static inline DWORD fl_protect(int, int);

EXPORT_SYMBOL void *mmap(void *start, size_t length, int prot, int flags,
    int fd, off_t offset)
{
	HANDLE filp, fmap;
	void *p;

	filp = reinterpret_cast(HANDLE, _get_osfhandle(fd));
	fmap = CreateFileMapping(filp, NULL, fl_protect(prot, flags),
	       0, 0, NULL);
	if (fmap == NULL)
		return MAP_FAILED;

	p = MapViewOfFile(fmap, dw_desired_access(prot, flags),
		((int64_t)offset >> 32) & 0xFFFFFFFFUL,
		offset & 0xFFFFFFFFUL, length);
	CloseHandle(fmap);
	if (p == NULL)
		return MAP_FAILED;

	return p;
}

EXPORT_SYMBOL int munmap(void *start, size_t length)
{
	if (!UnmapViewOfFile(start))
		return -1;
	return 0;
}

static inline DWORD dw_desired_access(int prot, int flags)
{
	if (flags & MAP_PRIVATE) return FILE_MAP_COPY;
	if (prot & PROT_WRITE)   return FILE_MAP_WRITE;
	if (prot & PROT_READ)	return FILE_MAP_READ;
#ifdef FILE_MAP_EXECUTE /* WinXP SP2 or WinServer2003 SP1 */
	if (prot & PROT_EXEC)	return FILE_MAP_EXECUTE;
#endif
	return 0;
}

static inline DWORD fl_protect(int prot, int flags)
{
	if (flags & MAP_PRIVATE)
		return PAGE_WRITECOPY;
#ifdef PAGE_EXECUTE_READ
	if (flags & (PROT_EXEC | PROT_READ))
		return PAGE_EXECUTE_READ;
#endif
#ifdef PAGE_EXECUTE_READWRITE
	if (flags & (PROT_EXEC | PROT_READ | PROT_WRITE))
		return PAGE_EXECUTE_READWRITE;
#endif
	if (flags & PROT_WRITE)
		return PAGE_READWRITE;
	if (flags & PROT_READ)
		return PAGE_READONLY;
	return 0;
}
