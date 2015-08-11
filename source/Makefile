ROOT 		 := ..
TARGETS  := $(ROOT)/libdoubletake.so
LIBS     := dl pthread
#CXXFLAGS := --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_MEMORY_LEAKS -DDEBUG_LEVEL=5 # leaks only
# CXXFLAGS := --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=5 # overflow only

# FIXME: BARRIER_SUPPORT
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DEVALUATING_PERF -DDEBUG_LEVEL=3#  -DDETECT_MEMORY_LEAKS
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=3 -DEVALUATING_PERF#  -DDETECT_MEMORY_LEAKS
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=3 -DEVALUATING_PERF
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=3 -DEVALUATING_PERF -DDETECT_MEMORY_LEAKS
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_MEMORY_LEAKS  -DDEBUG_LEVEL=3 -DEVALUATING_PERF
#CXXFLAGS := -O2 -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDEBUG_LEVEL=3 -DEVALUATING_PERF
CXXFLAGS := -Wall -DNDEBUG --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDEBUG_LEVEL=0 
#CXXFLAGS := -O2 -Wall -DNDEBUG --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDEBUG_LEVEL=3 -DEVALUATING_PERF -DDETECT_MEMORY_LEAKS -DGET_CHARECTERISTICS
#CXXFLAGS := -O2 -Wall -DNDEBUG --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=3 -DEVALUATING_PERF -DDETECT_MEMORY_LEAKS
#CXXFLAGS := -O2 -Wall -DNDEBUG --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror -DDETECT_OVERFLOW -DDEBUG_LEVEL=3 -DEVALUATING_PERF 
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror  -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDETECT_MEMORY_LEAKS -DDEBUG_LEVEL=0
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror  -DDETECT_OVERFLOW -DDETECT_USAGE_AFTER_FREE -DDETECT_MEMORY_LEAKS -DEVALUATING_PERF -DDEBUG_LEVEL=3
#CXXFLAGS := -Wall --std=c++11 -g -fPIC -I$(ROOT)/include -I$(ROOT)/heaplayers -fno-omit-frame-pointer -Werror  -DDETECT_OVERFLOW  
#  -DDEBUG_LEVEL=3 -DEVALUATING_PERF #  -DDETECT_MEMORY_LEAKS
# -O2 

# -DDEBUG_LEVEL=1 -DDEBUG_ROLLBACK

PREREQS  := $(ROOT)/heaplayers

include $(ROOT)/common.mk

#$(ROOT)/heaplayers:
#	git clone git://github.com/emeryberger/Heap-Layers $(ROOT)/heaplayers
