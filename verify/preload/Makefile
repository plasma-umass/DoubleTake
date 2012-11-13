CC = gcc
CXX= g++
CFLAGS = -m32 -Wall -g -O3 
LIBS = -lm -lrt -ldl

all:
	gcc -shared -fPIC -o liboverride.so override.c -ldl
	gcc -o test test.c

clean:
	rm -f lib* test


