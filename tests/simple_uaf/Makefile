ROOT    := ../..
TARGETS := simple_uaf
CFLAGS  := -std=c99 -g -fno-omit-frame-pointer
LDFLAGS := -L$(ROOT) -ldoubletake

include $(ROOT)/common.mk

test::
	@LD_LIBRARY_PATH=$(ROOT) ./simple_uaf
