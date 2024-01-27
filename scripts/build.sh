#!/bin/bash
set -e

cd ..
mkdir -p build/linux/"$1"
cd build/linux/"$1"
cmake -DCMAKE_BUILD_TYPE="$1" ../../..
make

cd ../../../scripts
