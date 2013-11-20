ROOT = ../..
TARGETS = simple_overflow
LIB_DIRS = $(ROOT)
LIBS = doubletake
CFLAGS += -g

include $(ROOT)/common.mk

test::
	@LD_LIBRARY_PATH=$(ROOT) ./simple_overflow
