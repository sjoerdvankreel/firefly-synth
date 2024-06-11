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

zip -r firefly_synth_1.7.9_mac_vst3_fx.zip firefly_synth_fx_1.vst3
zip -r firefly_synth_1.7.9_mac_vst3_instrument.zip firefly_synth_1.vst3
zip -r firefly_synth_1.7.9_mac_clap_fx.zip firefly_synth_fx_1.clap
zip -r firefly_synth_1.7.9_mac_clap_instrument.zip firefly_synth_1.clap

cd ../../../scripts
