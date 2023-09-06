#pragma once
#include <plugin_base/topo.hpp>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace infernal::base::vst3 {

class controller final:
public Steinberg::Vst::EditControllerEx1 {
  plugin_desc const _desc;
public: 
  controller(plugin_topo&& topo): _desc(std::move(topo)) {}
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
};

}
