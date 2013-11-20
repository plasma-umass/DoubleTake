ROOT = ../..
TARGETS = simple_leak
LIB_DIRS = $(ROOT)
LIBS = doubletake

include $(ROOT)/common.mk

test::
	@LD_LIBRARY_PATH=$(ROOT) ./simple_leak
