#!/bin/bash
set -e

cd ..
mkdir -p build/mac
cd build/mac
cmake -G Xcode ../..
Xcodebuild -configuration "$1"
 
