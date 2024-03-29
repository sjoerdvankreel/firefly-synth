#!/bin/bash
set -e

cd ..
mkdir -p build/mac/"$1"
cd build/mac/"$1"
cmake -DCMAKE_BUILD_TYPE="$1" ../../..
make

cd ../../../dist/"$1"/mac
./plugin_base.ref_gen firefly_synth_1.vst3/Contents/MacOS/firefly_synth_1.so ../../../param_reference.html
cd ../../../scripts
