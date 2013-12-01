#include <errno.h>
#include "internal.h"

EXPORT_SYMBOL int chown(const char *path, long uid, long gid)
{
	return -ENOSYS;
}

EXPORT_SYMBOL int fchmod(int fd, long perm)
{
	return -ENOSYS;
}

EXPORT_SYMBOL int fchown(int fd, long uid, long gid)
{
	return -ENOSYS;
}

EXPORT_SYMBOL int lchown(const char *path, long uid, long gid)
{
	return -ENOSYS;
}

EXPORT_SYMBOL int lstat(const char *path, struct stat *sb)
{
	return stat(path, sb);
}

EXPORT_SYMBOL int mkfifo(const char *path, long mode)
{
	return -EPERM;
}

EXPORT_SYMBOL int mknod(const char *path, long mode, long dev)
{
	return -EPERM;
}

EXPORT_SYMBOL int readlink(const char *path, char *dest, size_t len)
{
	return -EINVAL;
}

EXPORT_SYMBOL int symlink(const char *src, const char *dest)
{
	return -EPERM;
}
