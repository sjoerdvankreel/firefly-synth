#!/bin/bash
set -e

cd ..
mkdir -p build/linux/"$1"
cd build/linux/"$1"
cmake -DCMAKE_BUILD_TYPE="$1" ../../..
make

cd ../../../dist/"$1"/linux
./plugin_base.ref_gen firefly_synth_1.vst3/Contents/x86_64-linux/firefly_synth_1.so ../../../param_reference.html

tar -czvf firefly_synth_1.7.2_linux_vst3_fx.tar.gz firefly_synth_fx_1.vst3
tar -czvf firefly_synth_1.7.2_linux_vst3_instrument.tar.gz firefly_synth_1.vst3
tar -czvf firefly_synth_1.7.2_linux_clap_fx.tar.gz firefly_synth_fx_1.clap
tar -czvf firefly_synth_1.7.2_linux_clap_instrument.tar.gz firefly_synth_1.clap

cd ../../../scripts
