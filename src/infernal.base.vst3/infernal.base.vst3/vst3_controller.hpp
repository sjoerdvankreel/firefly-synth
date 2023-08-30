#ifndef INFERNAL_BASE_VST3_VST3_CONTROLLER_HPP
#define INFERNAL_BASE_VST3_VST3_CONTROLLER_HPP

#include <infernal.base/plugin_topo.hpp>
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace infernal::base::vst3 {

class vst3_controller:
public Steinberg::Vst::EditControllerEx1 {
  runtime_plugin_topo const _topo;
public: 
  vst3_controller(plugin_topo const& topo): _topo(topo) {}
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
};

}
#endif 