#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define BUFSIZE 256

#define TEST_INPUT "-rw-rw-r-- 1 tonyliu tonyliu  123 Jan  2 11:18 Heap_overflow_15.c -rw-rw-r-- 1 tonyliu tonyliu  923 Jan  2 11:18 heap_overflow_array_953.c -rw-rw-r-- 1 tonyliu tonyliu  930 Jan  2 11:18 heap_overflow_array_good_959.c -rw-rw-r-- 1 tonyliu tonyliu  821 Jan  2 11:18 heap_overflow_basic_good_1936.c -rw-rw-r-- 1 tonyliu tonyliu 1210 Jan  2 11:18 heap_overflow_cplx_1845.c-rw-rw-r-- 1 tonyliu tonyliu 1448 Jan  2 11:18 heap_overflow_cplx_2145.c-rw-rw-r-- 1 tonyliu tonyliu 1212 Jan  2 11:18 heap_overflow_cplx_good_1846.c-rw-rw-r-- 1 tonyliu tonyliu 1453 Jan  2 11:18 heap_overflow_cplx_good_2149.c-rw-rw-r-- 1 tonyliu tonyliu  926 Jan  2 11:18 HeapOverFlow_good_2134.c-rw-rw-r-- 1 tonyliu tonyliu 1447 Jan  2 11:18 heap_overflow_location_2147.c-rw-rw-r-- 1 tonyliu tonyliu 1212 Jan  2 11:18 heap_overflow_location_good_1848.c-rw-rw-r-- 1 tonyliu tonyliu 1453 Jan  2 11:18 heap_overflow_location_good_2148.c"

int main(int argc, char **argv) {
  fprintf(stderr, "innnnnnnnnnnnnnnnnnnnn\n");

  char *buf;
  fprintf(stderr, "innnnnnnnnnnnnnnnnnnnn22222\n");
  buf = (char *)malloc(BUFSIZE);
  fprintf(stderr, "buf is %p\n", buf);
  //strcpy(buf, argv[1]);
  strcpy(buf, TEST_INPUT);
}



