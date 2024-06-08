#!/bin/bash
set -e

cd ..
mkdir -p build/linux/"$1"
cd build/linux/"$1"
cmake -DCMAKE_BUILD_TYPE="$1" ../../..
make

cd ../../../dist/"$1"/linux
./plugin_base.ref_gen firefly_synth_1.vst3/Contents/x86_64-linux/firefly_synth_1.so ../../../param_reference.html

zip -r firefly_synth_1.7.8_linux_vst3_fx.zip firefly_synth_fx_1.vst3
zip -r firefly_synth_1.7.8_linux_vst3_instrument.zip firefly_synth_1.vst3
zip -r firefly_synth_1.7.8_linux_clap_fx.zip firefly_synth_fx_1.clap
zip -r firefly_synth_1.7.8_linux_clap_instrument.zip firefly_synth_1.clap

cd ../../../scripts
