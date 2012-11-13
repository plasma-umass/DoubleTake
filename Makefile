SRCS =  libstopgap.cpp \
	      libfuncs.cpp  \
	      xthread.cpp    \
	      dlmalloc.c     \
	      finetime.c     \
	      gnuwrapper.cpp

INCS =  xmapping.h     \
        xdefines.h     \
        atomic.h       \
        xplock.h       \
        xsync.h        \
        xthread.h      \
        xheap.h        \
        xoneheap.h     \
        sourcesharedheap.h       \
        xpheap.h     \
        privateheap.h  \
        internalheap.h \
	      xglobals.h     \
	      xmemory.h      \
	      xpageinfo.h    \
	      xpagestore.h   \
	      xrun.h         \
	      objectheader.h \
	      libfuncs.h    \
	      finetime.h     \
	      mm.h           \
        xcontext.h     \
        regioninfo.h   \
        selfmap.h

DEPS = $(SRCS) $(INCS)

CXX = g++ -g 

# Detection on 32bit
# CXX = g++ -DSSE_SUPPORT -m32 -DX86_32BIT -O3 -fno-omit-frame-pointer -DDETECT_FALSE_SHARING
# Detection on 64bit
#CXX = g++ -DSSE_SUPPORT -m64 -fno-omit-frame-pointer -DDETECT_FALSE_SHARING


# -march=core2 -msse3 -DSSE_SUPPORT 
#CFLAGS   = -Wall -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer
CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer
CFLAGS32 = $(CFLAGS) -m32 -DX86_32BIT -DDEBUG_LEVEL=3 # -O3
CFLAGS64 = $(CFLAGS) -DDEBUG_LEVEL=3 #-m64 # -O3
#CFLAGS64 = $(CFLAGS) -m64 # -O3

INCLUDE_DIRS = -I. -I./heaplayers -I./heaplayers/util

TARGETS = libstopgap32.so libstopgap64.so

all: $(TARGETS)

libstopgap32.so: $(DEPS)
	$(CXX) $(CFLAGS32) $(INCLUDE_DIRS) -shared -fPIC -D'CUSTOM_PREFIX(x)=stopgap_##x' $(SRCS) -o libstopgap32.so  -ldl -lpthread

libstopgap64.so: $(DEPS)
	$(CXX) $(CFLAGS64) $(INCLUDE_DIRS) -shared -fPIC -D'CUSTOM_PREFIX(x)=stopgap_##x' $(SRCS) -o libstopgap64.so  -ldl -lpthread

clean:
	rm -f $(TARGETS)

