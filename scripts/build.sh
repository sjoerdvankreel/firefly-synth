#!/bin/bash
set -e

cd ..
mkdir -p build/linux/debug
cd build/linux/debug
cmake -DCMAKE_BUILD_TYPE="$1" ../../..
make

cd ../../../scripts
