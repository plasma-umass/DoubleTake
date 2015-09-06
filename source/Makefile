ROOT 		 := ..
TARGETS  := $(ROOT)/libdoubletake.so
LIBS     := dl pthread
CXXFLAGS := -Wall -DNDEBUG --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDETECT_MEMORY_LEAKS -DDEBUG_LEVEL=0 

PREREQS  := $(ROOT)/heaplayers

include $(ROOT)/common.mk
