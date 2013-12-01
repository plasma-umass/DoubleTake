# -*- Makefile -*-

SUBDIRS = include

AM_CFLAGS        = ${regular_CFLAGS} ${GCC_FVISIBILITY_HIDDEN} -I${srcdir}/include
AM_CXXFLAGS      = ${regular_CXXFLAGS} ${GCC_FVISIBILITY_HIDDEN} -I${srcdir}/include

lib_LTLIBRARIES  = libHX.la
libHX_la_SOURCES = deque.c dir.c dl.c format.c map.c \
                   mc.c misc.c opt.c \
                   rand.c string.c time.c ${mingw32_extra} ${proc_sources}
libHX_la_LIBADD  = ${regular_LIBS}
libHX_la_LDFLAGS = -version-info 25:0:3

if MINGW32
mingw32_extra    = ux-file.c ux-mmap.c
endif
if B_PROC
proc_sources     = proc.c
endif

check_PROGRAMS     = tc-compile tc-cast tc-deque tc-dir tc-list \
                     tc-map tc-misc tc-option tc-proc tc-rand tc-string tc-time
tc_cast_LDADD      = libHX.la -lm
tc_compile_LDADD   = libHX.la
tc_dir_LDADD       = libHX.la
tc_list_LDADD      = libHX.la
tc_map_LDADD       = libHX.la -lm -lrt
tc_misc_LDADD      = libHX.la
tc_option_LDADD    = libHX.la
tc_proc_LDADD      = libHX.la
tc_rand_LDADD      = libHX.la -lrt
tc_string_LDADD    = libHX.la
tc_time_LDADD      = libHX.la -lrt

if HAVE_CXX
check_PROGRAMS    += tx-compile tx-cast tx-deque tx-dir tx-list \
                     tx-misc tx-option tx-proc tx-rand tx-string tx-time
tx_cast_SOURCES    = tx-cast.cpp
tx_cast_LDADD      = libHX.la -lm
tx_compile_SOURCES = tx-compile.cpp
tx_compile_LDADD   = libHX.la
tx_deque_SOURCES   = tx-deque.cpp
tx_dir_SOURCES     = tx-dir.cpp
tx_dir_LDADD       = libHX.la
tx_list_SOURCES    = tx-list.cpp
tx_list_LDADD      = libHX.la
tx_misc_SOURCES    = tx-misc.cpp
tx_misc_LDADD      = libHX.la
tx_option_SOURCES  = tx-option.cpp
tx_proc_SOURCES    = tx-proc.cpp
tx_proc_LDADD      = libHX.la
tx_rand_SOURCES    = tx-rand.cpp
tx_rand_LDADD      = libHX.la -lrt
tx_string_SOURCES  = tx-string.cpp
tx_string_LDADD    = libHX.la
tx_time_SOURCES    = tx-time.cpp
tx_time_LDADD      = libHX.la -lrt
endif
