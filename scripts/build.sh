#!/bin/bash
set -e

cd ..
mkdir -p build/linux/debug
cd build/linux/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../../..
make

if [ "$1" -eq "1" ] ; then
cd ..
mkdir -p release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ../../..
make
fi

cd ../../../scripts
