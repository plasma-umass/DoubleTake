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

OPTS    := -O3 -fomit-frame-pointer -fno-exceptions -fno-strict-aliasing -finline-limit=1800
#OPTS    += -frename-registers -finline-functions -funroll-loops
#OPTS    += -freduce-all-givs -fmove-all-movables -fgcse -fgcse-lm -fgcse-sm


CFLAGS = -g -O3 -Wall -funsigned-char $(OPTS)
CC = gcc
LEX = flex -I8
MAKEINFO = makeinfo
PACKAGE = bc
RANLIB = ranlib
READLINELIB = 
VERSION = 1.06
YACC = bison -y

bin_PROGRAMS = bc

bc_SOURCES = main.c bc.y scan.l execute.c load.c storage.c util.c global.c

EXTRA_DIST = bc.h bcdefs.h const.h fix-libmath_h global.h libmath.b proto.h              sbc.y

noinst_HEADERS = libmath.h

DISTCLEANFILES = sbc sbc.c sbc.h

MAINTAINERCLEANFILES = Makefile.in libmath.h bc.c bc.h scan.c

INCLUDES = -I$(srcdir) -I$(srcdir)/../h $(INCLS)
LIBBC = ../lib/libbc.a
LIBL = -lfl -rdynamic 
LDADD = $(LIBBC) $(LIBL) 

YFLAGS = -d


fbcOBJ = main.o bc.o scan.o execute.o global.o load.o storage.o util.o

sbcOBJ = main.o sbc.o scan.o execute.o global.o load.o storage.o util.o


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS = 
LDFLAGS = 
LIBS = 
bc_OBJECTS =  main.o bc.o scan.o execute.o load.o storage.o util.o \
global.o
bc_LDADD = $(LDADD)
bc_DEPENDENCIES =  ../lib/libbc.a
bc_LDFLAGS = 
LEX_OUTPUT_ROOT = lex.yy
LEXLIB = -lfl
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) $(DECLS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ 
HEADERS =  $(noinst_HEADERS)

TAR = tar
GZIP_ENV = --best
SOURCES = $(bc_SOURCES)
OBJECTS = $(bc_OBJECTS)

all: bc all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .l .o .s .y


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

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

bc: $(bc_OBJECTS) $(bc_DEPENDENCIES)
	@rm -f bc
	$(LINK) $(bc_LDFLAGS) $(bc_OBJECTS) $(MLOPTS) $(bc_LDADD) $(LIBS)
	$(LINK) $(bc_LDFLAGS) $(bc_OBJECTS) $(MLOPTS) $(bc_LDADD) $(LIBS) ../../../../../../libdoubletake.so -o bc-doubletake

.l.c:
	$(LEX) $(AM_LFLAGS) $(LFLAGS) $< && mv $(LEX_OUTPUT_ROOT).c $@
.y.c:
	$(YACC) $(AM_YFLAGS) $(YFLAGS) $< && mv y.tab.c $*.c
	if test -f y.tab.h; then \
	if cmp -s y.tab.h $*.h; then rm -f y.tab.h; else mv y.tab.h $*.h; fi; \
	else :; fi
bc.h: bc.c


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

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = bc

bc.o: bc.c bcdefs.h ../config.h const.h ../h/number.h global.h proto.h
execute.o: execute.c bcdefs.h ../config.h const.h ../h/number.h global.h \
	proto.h
global.o: global.c bcdefs.h ../config.h const.h ../h/number.h global.h \
	libmath.h
load.o: load.c bcdefs.h ../config.h const.h ../h/number.h global.h \
	proto.h
main.o: main.c bcdefs.h ../config.h const.h ../h/number.h global.h \
	proto.h ../h/getopt.h
scan.o: scan.c bcdefs.h ../config.h const.h ../h/number.h bc.h global.h \
	proto.h
storage.o: storage.c bcdefs.h ../config.h const.h ../h/number.h global.h \
	proto.h
util.o: util.c bcdefs.h ../config.h const.h ../h/number.h global.h \
	proto.h

info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am

all-am: Makefile $(PROGRAMS) $(HEADERS)
all-redirect: all-am

mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(DISTCLEANFILES)" || rm -f $(DISTCLEANFILES)

maintainer-clean-generic:
	-test -z "scanlbchbcc$(MAINTAINERCLEANFILES)" || rm -f scanl bch bcc $(MAINTAINERCLEANFILES)
mostlyclean-am:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-binPROGRAMS clean-compile clean-tags clean-generic \
		mostlyclean-am

clean: clean-am

distclean-am:  distclean-binPROGRAMS distclean-compile distclean-tags \
		distclean-generic clean-am

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-binPROGRAMS \
		maintainer-clean-compile maintainer-clean-tags \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir info-am info dvi-am dvi check \
check-am installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


$(PROGRAMS): $(LIBBC)

scan.o: bc.h
global.o: libmath.h

libmath.h: libmath.b
	echo '{0}' > libmath.h
	$(MAKE) fbc
	./fbc -c $(srcdir)/libmath.b </dev/null >libmath.h
	$(srcdir)/fix-libmath_h
	rm -f ./fbc
fbc: $(fbcOBJ)
	$(LINK) $(fbcOBJ) $(LIBBC) $(LIBL) $(READLINELIB) $(LIBS)
sbc.o: sbc.c
sbc: $(sbcOBJ)
	$(LINK) $(sbcOBJ) $(LIBBC) $(LIBL) $(READLINELIB) $(LIBS)

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
