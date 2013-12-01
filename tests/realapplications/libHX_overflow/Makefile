CC = gcc
CXX= g++
#CFLAGS = -m32 -Wall -g -O3
CFLAGS = -Wall -g -O1
#CFLAGS = -Wall -g -O1 -I./libHX-3.4/src/include/libHX/

LIBS = -lm -lrt -ldl

all:
	$(CC) $(CFLAGS) -o split split.c -L./libHX-3.4/src/.libs -lHX -rdynamic ../../../libdoubletake64.so
#	$(CXX) $(CFLAGS) -o split split.c -L./libHX-3.4/src/.libs -lHX -rdynamic ../../../libdoubletake64.so

clean:
	rm -f split
