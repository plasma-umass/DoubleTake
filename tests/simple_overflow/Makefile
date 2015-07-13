ROOT    := ../..
TARGETS := simple_overflow
CFLAGS  := -std=c99 -g
LDFLAGS := -L$(ROOT) -ldoubletake

include $(ROOT)/common.mk

test::
	@LD_LIBRARY_PATH=$(ROOT) ./simple_overflow
