#pragma once
#include <plugin_base/desc.hpp>
#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class editor;

class controller final:
public Steinberg::Vst::EditControllerEx1 {
  editor* _editor = {};
  plugin_desc const _desc;
  plugin_topo_factory const _topo_factory;
public: 
  plugin_desc const& desc() const { return _desc; }
  controller(plugin_topo_factory factory) : _desc(factory), _topo_factory(factory) {}
  
  Steinberg::IPlugView* PLUGIN_API createView(char const* name) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
};

}
