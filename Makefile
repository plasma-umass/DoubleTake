INCDIR = ./include
SRCDIR = ./source

SRCS =  $(SRCDIR)/libdoubletake.cpp \
	$(SRCDIR)/libfuncs.cpp      \
	$(SRCDIR)/xthread.cpp       \
	$(SRCDIR)/finetime.c        \
	$(SRCDIR)/dr.c              \
        $(SRCDIR)/xrun.cpp          \
	$(SRCDIR)/xmemory.cpp       \
	$(SRCDIR)/gnuwrapper.cpp    \
	$(SRCDIR)/quarantine.cpp    \
	$(SRCDIR)/internalsyncs.cpp \
	$(SRCDIR)/leakcheck.cpp     \
	$(SRCDIR)/selfmap.cpp       \
	$(SRCDIR)/prof.cpp       

INCS =  $(INCDIR)/xmapping.h      \
	$(INCDIR)/xdefines.h      \
	$(INCDIR)/atomic.h        \
	$(INCDIR)/xthread.h       \
	$(INCDIR)/xheap.h	  \
	$(INCDIR)/xoneheap.h      \
	$(INCDIR)/xpheap.h        \
	$(INCDIR)/xglobals.h      \
	$(INCDIR)/globalinfo.h    \
	$(INCDIR)/xmemory.h       \
	$(INCDIR)/xrun.h          \
	$(INCDIR)/objectheader.h  \
	$(INCDIR)/libfuncs.h      \
	$(INCDIR)/finetime.h      \
	$(INCDIR)/log.h           \
	$(INCDIR)/mm.h            \
	$(INCDIR)/xcontext.h      \
	$(INCDIR)/xsync.h         \
	$(INCDIR)/recordentries.h \
	$(INCDIR)/quarantine.h    \
	$(INCDIR)/freelist.h      \
	$(INCDIR)/sentinelmap.h   \
	$(INCDIR)/selfmap.h       \
	$(INCDIR)/bitmap.h        \
	$(INCDIR)/synceventlist.h \
	$(INCDIR)/watchpoint.h    \
	$(INCDIR)/leakcheck.h     \
	$(INCDIR)/syscalls.h      \
	$(INCDIR)/fops.h 

DEPS = $(SRCS) $(INCS)

CXX = clang++

CFLAGS = -m64 -g -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DDETECT_OVERFLOW -DDETECT_MEMORY_LEAKAGE -DEVALUATING_PERF -Wno-deprecated
# EDB: -O2 removed

# Overflow
#CFLAGS   = -g -O2 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DDETECT_OVERFLOW -DEVALUATING_PERF

# Memory use-after-free errors
#CFLAGS   = -g -O2 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DDETECT_USAGE_AFTER_FREE -DEVALUATING_PERF

# Memory Leakage
#CFLAGS   = -g -O2 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DDETECT_MEMORY_LEAKAGE -DEVALUATING_PERF

# Total
#CFLAGS   = -g -O2 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DDETECT_USAGE_AFTER_FREE -DDETECT_MEMORY_LEAKAGE -DDETECT_OVERFLOW -DEVALUATING_PERF
# -DMYDEBUG
#-DMULTI_THREAD
#-DDETECT_MEMORY_LEAKAGE
#-DDETECT_USAGE_AFTER_FREE
#-DDETECT_OVERFLOW
#-DREPRODUCIBLE_FDS #-m64 # -O3

CFLAGS64 = $(CFLAGS) -DDEBUG_LEVEL=0 -DDEBUG_ROLLBACK #-m64 # -O3

INCLUDE_DIRS = -I. -I$(INCDIR) -I./heaplayers -I./heaplayers/util

TARGETS = libdoubletake.so

all: $(TARGETS)

libdoubletake32.so: $(DEPS)
	$(CXX) $(CFLAGS32) $(INCLUDE_DIRS) -shared -fPIC -D'CUSTOM_PREFIX(x)=doubletake_##x' $(SRCS) -o libdoubletake32.so  -ldl -lpthread -lunwind -lunwind-ptrace -lunwind-generic

libdoubletake.so: $(DEPS)
	$(CXX) $(CFLAGS64) $(INCLUDE_DIRS) -shared -fPIC -D'CUSTOM_PREFIX(x)=doubletake_##x' $(SRCS) -o libdoubletake.so -ldl -lpthread

clean:
	rm -f $(TARGETS)


