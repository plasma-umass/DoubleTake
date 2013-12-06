#!/bin/sh

PWD=`pwd`;
PROCESS=$PWD"/bzip2-1.0.3/bzip2recover";
#echo "process is $PROCESS";
DIR=`perl -e 'print ((("x" x 200)."/") x 20)'`;
mkdir -p $DIR;
ln -s $PROCESS $DIR;
$DIR/bzip2recover;
FILE=$PWD"/xxxxx*";
rm -rf $FILE
