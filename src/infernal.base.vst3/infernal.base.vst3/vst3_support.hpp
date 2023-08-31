#pragma once
#include <infernal.base/plugin_topo.hpp>
#include <pluginterfaces/vst/vsttypes.h>

namespace infernal::base::vst3 {

double normalize(param_topo const& topo, param_value value);
param_value denormalize(param_topo const& topo, double value);
void to_vst_string(Steinberg::Vst::TChar* dest, int count, char const* source);

}
