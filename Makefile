ROOT := .
DIRS := source tests

include $(ROOT)/common.mk

format:
	@clang-format -i include/*.hh source/*.cpp
