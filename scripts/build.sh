#!/bin/bash
set -e

cd ..
mkdir -p build/linux/"$1"
cd build/linux/"$1"
cmake -DCMAKE_BUILD_TYPE="$1" ../../..
make

cd ../../../dist/"$1"/linux
./plugin_base.ref_gen ./firefly_synth_1.vst3/Contents/x86_64-linux/firefly_synth_1.vst3 ../../../PARAMREF.html
cd ../../../../scripts
