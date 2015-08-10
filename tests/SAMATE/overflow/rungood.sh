#!/bin/sh

./heap_overflow_array_good_959-dthread
echo "heap_overflow_array_good_959-dthread";
./heap_overflow_basic_good_1936-dthread 16
./heap_overflow_cplx_good_1846-dthread
./heap_overflow_cplx_good_2149-dthread
./HeapOverFlow_good_2134-dthread
./heap_overflow_location_good_1848-dthread
./heap_overflow_location_good_2148-dthread
./HeapOverflow_ArrayAddress_good_1959-dthread
./HeapOverflow_ArrayAddress_good_2068-dthread
./HeapOverflow_ArrayIndex_good_1962-dthread
./HeapOverflow_good_1953-dthread
./HeapOverflow_good_2066-dthread
./HeapOverflow_Scope_good_1956-dthread
./HeapOverflow_Scope_good_2067-dthread

