# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


srcdir = .

OPTS    := -O2 -fomit-frame-pointer -fno-exceptions -fno-strict-aliasing -finline-limit=1800
#OPTS    += -frename-registers -finline-functions -funroll-loops
#OPTS    += -freduce-all-givs -fmove-all-movables -fgcse -fgcse-lm -fgcse-sm

CC = gcc
LEX = flex -I8
MAKEINFO = makeinfo
PACKAGE = bc
RANLIB = ranlib
READLINELIB = 
VERSION = 1.06
YACC = bison -y

noinst_LIBRARIES = libbc.a

INCLUDES = -I. -I.. -I$(srcdir)/../h $(INCLS)

libbc_a_SOURCES = getopt.c getopt1.c vfprintf.c number.c

DEFS = -DHAVE_CONFIG_H $(DEFSADD)

CFLAGS = -g -O2 -Wall -funsigned-char $(OPTS) $(DECLS)

CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
LIBRARIES =  $(noinst_LIBRARIES)

CPPFLAGS = 
LDFLAGS = 
LIBS = 
libbc_a_LIBADD = 
libbc_a_OBJECTS =  getopt.o getopt1.o vfprintf.o number.o
AR = ar
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@

TAR = tar
GZIP_ENV = --best
SOURCES = $(libbc_a_SOURCES)
OBJECTS = $(libbc_a_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .o .s


mostlyclean-noinstLIBRARIES:

clean-noinstLIBRARIES:
	-test -z "$(noinst_LIBRARIES)" || rm -f $(noinst_LIBRARIES)

distclean-noinstLIBRARIES:

maintainer-clean-noinstLIBRARIES:

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

libbc.a: $(libbc_a_OBJECTS) $(libbc_a_DEPENDENCIES)
	-rm -f libbc.a
	$(AR) cru libbc.a $(libbc_a_OBJECTS) $(libbc_a_LIBADD)
	$(RANLIB) libbc.a

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

subdir = lib

getopt.o: getopt.c ../config.h ../h/getopt.h
getopt1.o: getopt1.c ../config.h ../h/getopt.h
number.o: number.c ../config.h ../h/number.h
vfprintf.o: vfprintf.c ../config.h

info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am

all-am: Makefile $(LIBRARIES)
all-redirect: all-am


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
mostlyclean-am:  mostlyclean-noinstLIBRARIES mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-noinstLIBRARIES clean-compile clean-tags clean-generic \
		mostlyclean-am

clean: clean-am

distclean-am:  distclean-noinstLIBRARIES distclean-compile \
		distclean-tags distclean-generic clean-am
#
distclean: distclean-am

maintainer-clean-am:  maintainer-clean-noinstLIBRARIES \
		maintainer-clean-compile maintainer-clean-tags \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-noinstLIBRARIES distclean-noinstLIBRARIES \
clean-noinstLIBRARIES maintainer-clean-noinstLIBRARIES \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir info-am info dvi-am dvi check \
check-am all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


newnumber.o: number.c muldigits.h
	$(CC) $(CFLAGS) $(INCLUDES) $(DECLS) -c -DMULDIGITS -o newnumber.o $(srcdir)/number.c

muldigits.h: testmul
	@echo "The following may take up to 10 minutes."
	testmul > muldigits.h

testmul: testmul.o number.o
	$(CC) $(CFLAGS) -o testmul testmul.o number.o

specialnumber: newnumber.o
	cp newnumber.o number.o

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
