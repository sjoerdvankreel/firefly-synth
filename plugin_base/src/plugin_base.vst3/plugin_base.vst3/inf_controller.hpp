#pragma once

#include <plugin_base/gui.hpp>
#include <plugin_base/desc.hpp>
#include <plugin_base/utility.hpp>

#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <utility>

namespace plugin_base::vst3 {

class inf_editor;

class inf_controller final:
public Steinberg::Vst::EditControllerEx1,
public gui_listener
{
  plugin_desc const _desc;
  inf_editor* _editor = {};
  jarray<plain_value, 4> _ui_state = {};

public: 
  INF_DECLARE_MOVE_ONLY(inf_controller);
  inf_controller(std::unique_ptr<plugin_topo>&& topo);

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& ui_state() { return _ui_state; }
  jarray<plain_value, 4> const& ui_state() const { return _ui_state; }
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }

  void gui_changing(int index, plain_value plain) override;
  void gui_end_changes(int index) override { endEdit(desc().param_index_to_tag[index]); }
  void gui_begin_changes(int index) override { beginEdit(desc().param_index_to_tag[index]); }

  Steinberg::IPlugView* PLUGIN_API createView(char const* name) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
};

}
