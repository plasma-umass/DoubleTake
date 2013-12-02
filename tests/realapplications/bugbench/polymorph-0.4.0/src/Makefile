
#
# Those are the compilations options for having the best performance
#
# If NATIVE is defined, the native gcc compiler is used. The same otimization options are used
# 
#

OPTS    := -g -O2 -fomit-frame-pointer -fno-exceptions -fno-strict-aliasing -finline-limit=1800 
#OPTS    += -frename-registers -finline-functions -funroll-loops
#OPTS    += -freduce-all-givs -fmove-all-movables -fgcse -fgcse-lm -fgcse-sm 

DECLS   += -DPACKAGE=\"polymorph\" -DVERSION=\"0.4.0\" -DHAVE_DIRENT_H=1 -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DHAVE_GETCWD=1 -DHAVE_STRCHR=1 -DHAVE_STRSTR=1 -I.

CFLAGS= $(OPTS)
CC    = gcc

OBJS	= polymorph.o llist.o rcfile.o 


.PHONY: polymorph

all: polymorph

polymorph: $(OBJS)
	$(CC) $(MOPTS) -o $@ $^ $(MLOPTS) -rdynamic /nfs/cm/scratch1/tonyliu/grace/branches/DoubleTake/libdoubletake64.so
	rm -rf *.o

.c.o:
	$(CC) $(CFLAGS) $(INCLS) $(DECLS) -c $<


clean:
	rm -rf *.s *.o *.c.t* ttt.* ttt 
