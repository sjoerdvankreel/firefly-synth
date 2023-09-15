#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/utility.hpp>

#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class vst_editor;

class vst_controller final:
public Steinberg::Vst::EditControllerEx1 {
  plugin_desc const _desc;
  vst_editor* _editor = {};
public: 
  plugin_desc const& desc() const { return _desc; }
  vst_controller(std::unique_ptr<plugin_topo>&& topo) : _desc(std::move(topo)) {}
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }

  Steinberg::IPlugView* PLUGIN_API createView(char const* name) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
};

}
