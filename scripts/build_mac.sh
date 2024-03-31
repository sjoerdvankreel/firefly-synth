#!/bin/bash
set -e

cd ..
mkdir -p build/mac/"$1"
cd build/mac/"$1"
export XCODE_VERSION="15" 
cmake -DCMAKE_BUILD_TYPE="$1" -DXCODE_VERSION="15" ../../..
make

cd ../../../dist/"$1"/mac
./plugin_base.ref_gen firefly_synth_1.vst3/Contents/MacOS/firefly_synth_1 ../../../param_reference.html

tar -czvf firefly_synth_1.6.0_mac_vst3_fx.tar.gz firefly_synth_fx_1.vst3
tar -czvf firefly_synth_1.6.0_mac_vst3_instrument.tar.gz firefly_synth_1.vst3
tar -czvf firefly_synth_1.6.0_mac_clap_fx.tar.gz firefly_synth_fx_1.clap
tar -czvf firefly_synth_1.6.0_mac_clap_instrument.tar.gz firefly_synth_1.clap

cd ../../../scripts
