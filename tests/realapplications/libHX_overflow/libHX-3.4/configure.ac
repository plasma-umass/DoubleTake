
AC_INIT([libHX], [3.4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_INSTALL
AM_INIT_AUTOMAKE([-Wall -Wno-override foreign subdir-objects])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL

#
# Check whether there really is a C++ compiler.
# It is not mandatory to compile libHX, but we want to know.
#
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [return 0;])],
	[ac_cv_cxx=yes], [ac_cv_cxx=no])
AC_LANG_POP([C++])
AM_CONDITIONAL([HAVE_CXX], [test "$ac_cv_cxx" = yes])

AC_MSG_CHECKING([for mingw32])
AC_LANG_PUSH([C])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([], [return __MINGW32__;])],
	[ac_cv_mingw32=yes; AC_MSG_RESULT([yes])],
	[ac_cv_mingw32=no; AC_MSG_RESULT([no])]
)
AC_LANG_POP([C])
AM_CONDITIONAL([MINGW32], [test "$ac_cv_mingw32" = yes])

AC_ARG_WITH([pkgconfigdir], AS_HELP_STRING([--with-pkgconfigdir=PATH],
	[Path to the pkgconfig directory [[LIBDIR/pkgconfig]]]),
	[pkgconfigdir="$withval"], [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST([pkgconfigdir])

CHECK_GCC_FVISIBILITY
AC_CHECK_LIB([dl], [dlopen], [regular_LIBS="$regular_LIBS -ldl"])

AC_CHECK_MEMBERS(
	[struct timespec.tv_nsec,
	struct timeval.tv_usec,
	struct stat.st_mtimensec, struct stat.st_mtim,
	struct stat.st_mtimespec, struct stat.st_mtime,
	struct stat.st_otimensec, struct stat.st_otim,
	struct stat.st_otimespec, struct stat.st_otime],
	[], [], [
	#include <sys/stat.h>
	#include <sys/time.h>
	#include <time.h>
	])
AC_CHECK_FUNCS([fork execv execvp pipe], [b_proc="1"])
AC_CHECK_FUNCS([gettimeofday getegid geteuid getpid getppid])
AM_CONDITIONAL([B_PROC], [test "$b_proc" = 1])

regular_CFLAGS="-D_FILE_OFFSET_BITS=64 \
	-D_REENTRANT -Wall -Waggregate-return -Wmissing-declarations \
	-Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes \
	-Wformat=2 -pipe"
regular_CXXFLAGS="-D_FILE_OFFSET_BITS=64 \
	-D_REENTRANT -Wall -Wno-pointer-arith -Wredundant-decls -pipe";
AC_SUBST([regular_CFLAGS])
AC_SUBST([regular_CXXFLAGS])
AC_SUBST([regular_LIBS])
AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile
	src/include/Makefile libHX.pc])
AC_OUTPUT
