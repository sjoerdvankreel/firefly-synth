#!/bin/bash
set -e

cd ..
mkdir -p build/mac_xcode
cd build/mac_xcode
cmake -G Xcode ../..
 
