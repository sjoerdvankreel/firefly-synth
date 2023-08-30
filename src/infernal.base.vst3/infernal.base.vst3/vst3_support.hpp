#ifndef INFERNAL_BASE_VST3_VST3_SUPPORT_HPP
#define INFERNAL_BASE_VST3_VST3_SUPPORT_HPP

#include <infernal.base/plugin_topo.hpp>
#include <pluginterfaces/vst/vsttypes.h>

namespace infernal::base::vst3 {

double normalize(param_topo const& topo, param_value value);
param_value denormalize(param_topo const& topo, double value);
void copy_to_vst_string(Steinberg::Vst::TChar* dest, int count, char const* source);

}
#endif 