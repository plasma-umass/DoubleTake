SRCS =  libstopgap.cpp \
	      libfuncs.cpp  \
	      xthread.cpp    \
	      finetime.c     \
        dr.c           \
        xrun.cpp       \
        xmemory.cpp    \
	      gnuwrapper.cpp \
				prof.cpp       

INCS =  xmapping.h     \
        xdefines.h     \
        atomic.h       \
        xthread.h      \
        xheap.h        \
        xoneheap.h     \
        xpheap.h     \
	      xglobals.h     \
	      xmemory.h      \
	      xrun.h         \
	      objectheader.h \
	      libfuncs.h    \
	      finetime.h     \
	      mm.h           \
        xcontext.h     \
        regioninfo.h   \
        selfmap.h      \
        bitmap.h       \
        sanitycheck.h  \
        watchpoint.h \
        syscalls.h \
        fops.h 

DEPS = $(SRCS) $(INCS)

CXX = g++ 
#-D_GNU_SOURCE 

# Detection on 32bit
# CXX = g++ -DSSE_SUPPORT -m32 -DX86_32BIT -O3 -fno-omit-frame-pointer -DDETECT_FALSE_SHARING
# Detection on 64bit
#CXX = g++ -DSSE_SUPPORT -m64 -fno-omit-frame-pointer -DDETECT_FALSE_SHARING


# -march=core2 -msse3 -DSSE_SUPPORT 
#CFLAGS   = -Wall -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DSINGLE_THREAD 
#-DHANDLE_SYSCALL
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL -DSINGLE_THREAD
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DSINGLE_THREAD
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DHANDLE_SYSCALL -DSINGLE_THREAD
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL -DSINGLE_THREAD
CFLAGS   = -g -O0 -DSSE_SUPPORT -fno-omit-frame-pointer -DMULTI_THREAD -DHANDLE_SYSCALL
#-DDETECT_OVERFLOW
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_OVERFLOW -DDETECT_RACES -DHANDLE_SYSCALL -DMULTI_THREAD
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL -DSINGLE_THREAD -DSTOP_AT_OVERFLOW
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL -DPROTECT_MEMORY -DSINGLE_THREAD

#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DSINGLE_THREAD -DDETECT_NONALIGNED_OVERFLOW -DHANDLE_SYSCALL
#CFLAGS   = -msse3 -DSSE_SUPPORT -fno-omit-frame-pointer -DSINGLE_THREAD 
#-DDETECT_NONALIGNED_OVERFLOW
CFLAGS32 = $(CFLAGS) -m32 -DX86_32BIT -DDEBUG_LEVEL=3 # -O3
#CFLAGS64 = $(CFLAGS) -DDEBUG_LEVEL=0 #-m64 # -O3
#-DREPRODUCIBLE_FDS # whether we should care about the reprocibilities of fds.
# That is, the second run may have the different fds with the first run.
# So if the fd is not contributed to the buffer overflow, then we can still detect and 
# reproduce it.
#CFLAGS64 = $(CFLAGS) -DDEBUG_LEVEL=0 -DDEBUG_ROLLBACK -DREPRODUCIBLE_FDS #-m64 # -O3
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


