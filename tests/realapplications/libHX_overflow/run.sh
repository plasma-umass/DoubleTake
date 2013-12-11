#!/bin/sh

#cd ./libHX-3.4
#make clean
#./configure
#make
#cd ../
make
LD_LIBRARY_PATH=./libHX-3.4/src/.libs ./split
