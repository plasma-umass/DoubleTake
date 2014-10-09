ROOT    := ../..
TARGETS := simple_uaf
CFLAGS  := -g
LDFLAGS := -L$(ROOT) -ldoubletake

include $(ROOT)/common.mk

test::
	@LD_LIBRARY_PATH=$(ROOT) ./simple_uaf
