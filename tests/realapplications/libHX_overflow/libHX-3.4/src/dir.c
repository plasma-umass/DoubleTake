/*
 *	File and directory handling
 *	Copyright © Jan Engelhardt <jengelh [at] medozas de>, 2002 - 2008
 *
 *	This file is part of libHX. libHX is free software; you can
 *	redistribute it and/or modify it under the terms of the GNU
 *	Lesser General Public License as published by the Free Software
 *	Foundation; either version 2.1 or 3 of the License.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined _WIN32
#	include <direct.h>
#	include <io.h>
#	include <windows.h>
#else
#	include <dirent.h>
#	include <unistd.h>
#endif
#include <libHX/ctype_helper.h>
#include <libHX/misc.h>
#include <libHX/string.h>
#include "internal.h"

struct HXdir {
#if defined _WIN32
	char *dname;
	HANDLE ptr;
	WIN32_FIND_DATA dentry;
	bool got_first;
#else
	DIR *ptr;
	/*
	 * Surprise surprise, along comes Solaris with a dirent too short!
	 */
	union {
		struct dirent dentry;
		char extender[_POSIX_PATH_MAX + sizeof(struct dirent) - 
			HXsizeof_member(struct dirent, d_name)];
	};
#endif
};

static int mkdir_gen(const char *d)
{
	struct stat sb;
	if (lstat(d, &sb) < 0) {
#if defined(_WIN32)
		if (mkdir(d) < 0)
#else
		if (mkdir(d, 0777) < 0) /* use umask() for permissions */
#endif
			return -errno;
	} else {
#if defined(_WIN32)
		if ((sb.st_mode & S_IFDIR) != S_IFDIR)
#else
		if (!S_ISDIR(sb.st_mode))
#endif
			return -errno;
	}
	return 1;
}

EXPORT_SYMBOL void *HXdir_open(const char *s)
{
	struct HXdir *d;
	if ((d = malloc(sizeof(struct HXdir))) == NULL)
		return NULL;

#if defined(_WIN32)
	if ((d->dname = malloc(strlen(s) + 3)) == NULL)
		goto out;
	strcpy(d->dname, s);
	strcat(d->dname, "\\*");
	d->got_first = false;
#else
	if ((d->ptr = opendir(s)) == NULL)
		goto out;
#endif

	return d;
 out:
	free(d);
	return NULL;
}

EXPORT_SYMBOL const char *HXdir_read(void *dv)
{
	struct HXdir *d = dv;
	if (d == NULL)
		return NULL;

	errno = 0;
#if defined _WIN32
	if (!d->got_first) {
		d->got_first = true;
		if ((d->ptr = FindFirstFile(d->dname, &d->dentry)) == NULL)
			return NULL;
	} else if (!FindNextFile(d->ptr, &d->dentry)) {
		return NULL;
	}
	return d->dentry.cFileName;
#else
	{
		struct dirent *checkptr;
		int i = readdir_r(d->ptr, &d->dentry, &checkptr);
		if (checkptr == NULL || i < 0)
			return NULL;
	}
	return d->dentry.d_name;
#endif
}

EXPORT_SYMBOL void HXdir_close(void *dv)
{
	struct HXdir *d = dv;
	if (dv == NULL)
		return;
#if defined _WIN32
	FindClose(d->ptr);
	free(d->dname);
#else
	closedir(d->ptr);
#endif
	free(d);
}

EXPORT_SYMBOL int HX_copy_file(const char *src, const char *dest,
    unsigned int opts, ...)
{
	char buf[MAXLNLEN];
	unsigned int extra_flags = 0;
	int dd, eax = 0, sd, l;
	int throw_away;

	if ((sd = open(src, O_RDONLY | O_BINARY)) < 0)
		return -errno;
	if (opts & HXF_KEEP)
		extra_flags = O_EXCL;
	dd = open(dest, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC |
	     extra_flags, S_IRUGO | S_IWUGO);
	if (dd < 0) {
		eax = errno;
		close(sd);
		errno = eax;
		if (extra_flags != 0 && eax == EEXIST)
			return 1;
		return -errno;
	}

	while ((l = read(sd, buf, MAXLNLEN)) > 0 && write(dd, buf, l) > 0)
		;
	close(sd);

	if (opts & (HXF_UID | HXF_GID)) {
		struct stat sb;
		long uid, gid;
		va_list argp;
		va_start(argp, opts);

		fstat(dd, &sb);
		uid = sb.st_uid;
		gid = sb.st_gid;

		if (opts & HXF_UID) uid = va_arg(argp, long);
		if (opts & HXF_GID) gid = va_arg(argp, long);
		throw_away = fchown(dd, uid, gid);
		va_end(argp);
	}
	close(dd);
	return 1;
}

EXPORT_SYMBOL int HX_copy_dir(const char *src, const char *dest,
    unsigned int opts, ...)
{
	void *dt = HXdir_open(src);
	long uid = -1, gid = -1;
	const char *fn;
	int throw_away;

	if (dt == NULL)
		return 0;

	{
		va_list argp;
		va_start(argp, opts);
		if (opts & HXF_UID) uid = va_arg(argp, long);
		if (opts & HXF_GID) gid = va_arg(argp, long);
		va_end(argp);
	}

	while ((fn = HXdir_read(dt)) != NULL) {
		char fsrc[MAXFNLEN], fdest[MAXFNLEN];
		struct stat sb;

		if (strcmp(fn, ".") == 0 || strcmp(fn, "..") == 0)
			continue;
		snprintf(fsrc,  MAXFNLEN, "%s/%s", src,  fn);
		snprintf(fdest, MAXFNLEN, "%s/%s", dest, fn);

		lstat(fsrc, &sb);
		sb.st_mode &= 0777; /* clear SUID/GUID/Sticky bits */

		if (S_ISREG(sb.st_mode)) {
			HX_copy_file(fsrc, fdest, opts, uid, gid);
		} else if (S_ISDIR(sb.st_mode)) {
			HX_mkdir(fdest);
			HX_copy_dir(fsrc, fdest, opts, uid, gid);
		} else if (S_ISLNK(sb.st_mode)) {
			char pt[MAXFNLEN];
			memset(pt, '\0', MAXFNLEN);
			if (readlink(fsrc, pt, MAXFNLEN - 1) < MAXFNLEN - 1)
				throw_away = symlink(pt, fdest);
		} else if (S_ISBLK(sb.st_mode) || S_ISCHR(sb.st_mode)) {
			mknod(fdest, sb.st_mode, sb.st_dev);
		} else if (S_ISFIFO(sb.st_mode)) {
			mkfifo(fdest, sb.st_mode);
		}

		throw_away = lchown(fdest, uid, gid);
		if (!S_ISLNK(sb.st_mode))
			chmod(fdest, sb.st_mode);
	}

	HXdir_close(dt);
	return 1;
}

EXPORT_SYMBOL int HX_mkdir(const char *idir)
{
	int i = 0, len = strlen(idir);
	char buf[MAXFNLEN], dir[MAXFNLEN];

#if defined(_WIN32)
	{
		char *p = dir;
		HX_strlcpy(dir, idir, sizeof(dir));
		for (; *p != '\0'; ++p)
			if (*p == '\\')
				*p = '/';
		if (HX_isalpha(dir[0]) && dir[1] == ':')
			i = 2;
	}
#else
	HX_strlcpy(dir, idir, sizeof(dir));
#endif

	if (dir[i] == '/')
		++i;
	for (; i < len; ++i) {
		int v;
		if (dir[i] == '/') {
			strncpy(buf, dir, i);
			buf[i] = '\0';
			if ((v = mkdir_gen(buf)) <= 0)
				return v;
		} else if (i == len - 1) {
			strncpy(buf, dir, len);
			buf[len] = '\0';
			if ((v = mkdir_gen(buf)) <= 0)
				return v;
		}
	}
	return 1;
}

EXPORT_SYMBOL int HX_rrmdir(const char *dir)
{
	struct HXdir *ptr;
	const char *trav;
	hxmc_t *fn = NULL;
	int ret = 0;

	if ((ptr = HXdir_open(dir)) == NULL)
		return -errno;

	while ((trav = HXdir_read(ptr)) != NULL) {
		struct stat sb;

		if (strcmp(trav, ".") == 0 || strcmp(trav, "..") == 0)
			continue;
		HXmc_strcpy(&fn, dir);
		HXmc_strcat(&fn, "/");
		HXmc_strcat(&fn, trav);
		if (lstat(fn, &sb) < 0) {
			if (ret == 0)
				ret = -errno;
			continue;
		}

		if (S_ISDIR(sb.st_mode)) {
			if (HX_rrmdir(fn) <= 0) {
				if (ret == 0)
					ret = -errno;
				continue;
			}
		} else if (unlink(fn) < 0) {
			if (ret == 0)
				ret = -errno;
			continue;
		}
	}

	if (rmdir(dir) < 0) {
		if (ret == 0)
			ret = -errno;
	}
	HXdir_close(ptr);
	HXmc_free(fn);
	return ret;
}
