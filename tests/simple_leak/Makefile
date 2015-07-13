ROOT    := ../..
TARGETS := simple_leak
CFLAGS  := -g
LDFLAGS := -L$(ROOT) -ldoubletake

include $(ROOT)/common.mk

test::
	@printenv
	@LD_LIBRARY_PATH=$(ROOT) ./simple_leak
