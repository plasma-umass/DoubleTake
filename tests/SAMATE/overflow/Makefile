CC = gcc
CXX= g++
CFLAGS = -Wall -g -O0 -fno-stack-protector -fno-mudflap -D_FORTIFY_SOURCE=0 
CXXFLAGS = $(CFLAGS)
LIBS = -lm -lrt -ldl
DOUBLETAKE_LIBS += $(LIBS) -rdynamic ../../../libdoubletake.so

SRCS := $(wildcard *.c)
TESTS := $(SRCS:.c=)
PTHREAD_OBJS_C := $(addsuffix -pthread, $(TESTS))
DOUBLETAKE_OBJS_C := $(addsuffix -doubletake, $(TESTS))

CPPSRCS := $(wildcard *.cpp)
CPPTESTS := $(CPPSRCS:.cpp=)
PTHREAD_OBJS_CPP := $(addsuffix -pthread, $(CPPTESTS))
DOUBLETAKE_OBJS_CPP := $(addsuffix -doubletake, $(TESTS))

.PHONY : default all clean
default: all
all: $(PTHREAD_OBJS) $(DOUBLETAKE_OBJS_C) $(DOUBLETAKE_OBJS_CPP)
clean:
	rm -f $(PTHREAD_OBJS) $(DOUBLETAKE_OBJS_C) $(DOUBLETAKE_OBJS_CPP) 

%-pthread:%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(PTHREAD_LIBS)

%-doubletake:%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(DOUBLETAKE_LIBS)

%-pthread:%.c
	$(CC) $(CFLAGS) -o $@ $< $(PTHREAD_LIBS)

%-dthread:%.c
	$(CC) $(CFLAGS) -o $@ $< $(DOUBLETAKE_LIBS)


