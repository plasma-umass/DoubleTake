ROOT 		 := ..
TARGETS  := $(ROOT)/libdoubletake.so
LIBS     := dl pthread
#CXXFLAGS := --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_MEMORY_LEAKS -DDEBUG_LEVEL=5 # leaks only
# CXXFLAGS := --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=5 # overflow only
CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDETECT_MEMORY_LEAKS -DDEBUG_LEVEL=5
# -O2 

# -DDEBUG_LEVEL=1 -DDEBUG_ROLLBACK

PREREQS  := $(ROOT)/heaplayers

include $(ROOT)/common.mk

$(ROOT)/heaplayers:
	git clone git://github.com/emeryberger/Heap-Layers $(ROOT)/heaplayers
