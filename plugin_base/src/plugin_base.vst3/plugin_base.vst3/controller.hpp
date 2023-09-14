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
public: 
  plugin_desc const& desc() const { return _desc; }
  controller(std::unique_ptr<plugin_topo>&& topo) : _desc(std::move(topo)) {}
  
  Steinberg::IPlugView* PLUGIN_API createView(char const* name) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }
};

}
