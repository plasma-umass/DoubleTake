ROOT 		 := ..
TARGETS  := $(ROOT)/libdoubletake.so
LIBS     := dl pthread
CXXFLAGS := -g -O2 -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror \
						-DDEBUG_LEVEL=1 -DDEBUG_ROLLBACK -DSSE_SUPPORT -DHANDLE_SYSCALL -DMULTI_THREAD \
						-DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDETECT_MEMORY_LEAKAGE
PREREQS  := $(ROOT)/heaplayers

include $(ROOT)/common.mk

$(ROOT)/heaplayers:
	git clone git://github.com/emeryberger/Heap-Layers $(ROOT)/heaplayers
