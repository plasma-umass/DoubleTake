SRCS =  libstopgap.cpp \
	      libfuncs.cpp  \
	      xthread.cpp    \
	      finetime.c     \
        dr.c           \
        xrun.cpp       \
        xmemory.cpp    \
	      gnuwrapper.cpp \
				quarantine.cpp  \
        internalsyncs.cpp \
				leakcheck.cpp \
				selfmap.cpp \
				prof.cpp       

INCS =  xmapping.h     \
        xdefines.h     \
        atomic.h       \
        xthread.h      \
        xheap.h        \
        xoneheap.h     \
        xpheap.h     \
	      xglobals.h     \
	      globalinfo.h     \
	      xmemory.h      \
	      xrun.h         \
	      objectheader.h \
	      libfuncs.h    \
	      finetime.h     \
        log.h         \
	      mm.h           \
        xcontext.h     \
				xsync.h  \
				recordentries.h \
				quarantine.h \
				freelist.h   \
				sentinelmap.h \
        selfmap.h      \
        bitmap.h       \
        synceventlist.h \
        watchpoint.h \
				leakcheck.h \
        syscalls.h \
        fops.h 

DEPS = $(SRCS) $(INCS)

CXX = g++ 
#-D_GNU_SOURCE 

# -march=core2 -msse3 -DSSE_SUPPORT
# When we evaluate performance, there is no need to rollback.
#-DEVALUATING_PERF

# Framework
#CFLAGS   = -g -O2 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DEVALUATING_PERF
CFLAGS   = -g -O2 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DDETECT_OVERFLOW -DDETECT_MEMORY_LEAKAGE -DEVALUATING_PERF

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

#INCLUDE_DIRS = -I. -I./Heap-Layers
INCLUDE_DIRS = -I. -I./heaplayers -I./heaplayers/util

TARGETS = libstopgap64.so

all: $(TARGETS)

libstopgap32.so: $(DEPS)
	$(CXX) $(CFLAGS32) $(INCLUDE_DIRS) -shared -fPIC -D'CUSTOM_PREFIX(x)=stopgap_##x' $(SRCS) -o libstopgap32.so  -ldl -lpthread -lunwind -lunwind-ptrace -lunwind-generic

#gcc -fPIC -std=c99 -c pthread_create.c
libstopgap64.so: $(DEPS)
	$(CXX) $(CFLAGS64) $(INCLUDE_DIRS) -shared -fPIC -D'CUSTOM_PREFIX(x)=stopgap_##x' $(SRCS) -o libstopgap64.so -ldl -lpthread

clean:
	rm -f $(TARGETS)


