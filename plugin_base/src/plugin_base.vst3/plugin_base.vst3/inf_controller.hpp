#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/utility.hpp>

#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class inf_editor;

class inf_controller final:
public Steinberg::Vst::EditControllerEx1 {
  plugin_desc const _desc;
  inf_editor* _editor = {};
  jarray<plain_value, 4> _ui_state = {};

public: 
  INF_DECLARE_MOVE_ONLY(inf_controller);
  inf_controller(std::unique_ptr<plugin_topo>&& topo);
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& ui_state() { return _ui_state; }
  jarray<plain_value, 4> const& ui_state() const { return _ui_state; }
  Steinberg::IPlugView* PLUGIN_API createView(char const* name) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
};

}
