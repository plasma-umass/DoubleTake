ROOT = ..
TARGETS = $(ROOT)/libdoubletake.so
INCLUDE_DIRS = $(ROOT)/include $(ROOT)/heaplayers $(ROOT)/heaplayers/util

CFLAGS = -fno-omit-frame-pointer -Wno-deprecated \
  -DSSE_SUPPORT -DHANDLE_SYSCALL -DDETECT_OVERFLOW -DDETECT_MEMORY_LEAKAGE -DEVALUATING_PERF

include $(ROOT)/common.mk
