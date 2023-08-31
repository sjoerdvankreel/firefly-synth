#pragma once
#include <infernal.base/plugin_topo.hpp>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace infernal::base::vst3 {

class vst3_controller final:
public Steinberg::Vst::EditControllerEx1 {
  runtime_plugin_topo const _topo;
public: 
  vst3_controller(plugin_topo&& topo): _topo(std::move(topo)) {}
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
};

}
