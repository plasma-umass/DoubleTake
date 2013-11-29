CC = gcc
CXX= g++
CFLAGS = -Wall -g -O2
#-T../doubletakes.ld
#CFLAGS = -Wall -g -O3 -T../doubletakes64.ld
LIBS = -lm -lrt -ldl

SRCS := $(wildcard *.cpp)
TESTS := $(SRCS:.c=)

SRCS := $(wildcard *.cpp)
TESTS := $(SRCS:.cpp=)

PTHREAD_LIBS += $(LIBS) -lpthread
PTHREAD_OBJS := $(addsuffix -pthread, $(TESTS))

#DOUBLETAKE_LIBS = $(LIBS) -rdynamic ../source-no-determsync/libdoubletake.so
#DOUBLETAKE_LIBS = $(LIBS) -rdynamic ../libstopgap32.so
#DOUBLETAKE_LIBS = $(LIBS) -L../ -lstopgap64
DOUBLETAKE_LIBS = $(LIBS) -rdynamic ../../libdoubletake64.so
#DOUBLETAKE_LIBS = $(LIBS) -rdynamic /nfs/cm/scratch1/tonyliu/grace/branches/determ/libgrace.so
#DOUBLETAKE_LIBS = $(LIBS) -rdynamic /nfs/cm/scratch1/tonyliu/grace/branches/stopgap/libgrace.so
DOUBLETAKE_OBJS := $(addsuffix -doubletake, $(TESTS))

COMPILE = $(CC) $(CFLAGS) 
COMPILE = $(CXX) $(CFLAGS) 

.PHONY : default all clean
default: all
all:$(PTHREAD_OBJS) $(DOUBLETAKE_OBJS)
clean:
	rm -f $(PTHREAD_OBJS) $(DOUBLETAKE_OBJS) 

%-pthread:%.c
	$(COMPILE) -o $@ $< $(PTHREAD_LIBS)

%-doubletake:%.c
	$(COMPILE) -o $@ $< $(DOUBLETAKE_LIBS)

%-pthread:%.cpp
	$(COMPILE) -o $@ $< $(PTHREAD_LIBS)

%-doubletake:%.cpp
	$(COMPILE) -o $@ $< $(DOUBLETAKE_LIBS)

