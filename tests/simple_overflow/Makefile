all:
	mkdir -p obj
	clang -std=c99 -g -MMD -MP -o obj/main.o -c main.c
	clang -std=c99 -g -MMD -MP -o obj/overflow.o -c overflow.c
	clang++ -rdynamic ../../libdoubletake.so  -o simple_overflow obj/main.o obj/overflow.o
#	clang++ -rdynamic /nfs/cm/scratch1/tonyliu/grace/branches/DoubleTake/DoubleTakeCharlie/DoubleTake/libdoubletake.so  -o simple_overflow obj/main.o obj/overflow.o

test::
	./simple_overflow

clean:
	rm -rf obj/* 
