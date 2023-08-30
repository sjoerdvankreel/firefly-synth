#ifndef INFERNAL_BASE_VST3_VST3_COMPONENT_HPP
#define INFERNAL_BASE_VST3_VST3_COMPONENT_HPP

#include <infernal.base/plugin_topo.hpp>
#include <infernal.base/plugin_engine.hpp>

#include <public.sdk/source/vst/vstaudioeffect.h>

namespace infernal::base::vst3 {

class vst3_component:
public Steinberg::Vst::AudioEffect {
  plugin_topo const _topo;
  runtime_top
};

}
#endif 