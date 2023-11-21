#pragma once

#include <plugin_base/gui/gui.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/utility.hpp>

#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <map>
#include <utility>

namespace plugin_base::vst3 {

class inf_editor;

class inf_controller final:
public Steinberg::Vst::EditControllerEx1,
public Steinberg::Vst::IMidiMapping,
public gui_listener
{
  inf_editor* _editor = {};
  plugin_state _gui_state = {};
  extra_state _extra_state;
  std::map<int, int> _midi_id_to_param = {};

public: 
  OBJ_METHODS(inf_controller, EditControllerEx1)
  DEFINE_INTERFACES
    DEF_INTERFACE(IMidiMapping)
  END_DEFINE_INTERFACES(EditControllerEx1)
  REFCOUNT_METHODS(EditControllerEx1)
  INF_PREVENT_ACCIDENTAL_COPY(inf_controller);

  inf_controller(plugin_desc const* desc): 
  _gui_state(desc, true), 
  _extra_state(gui_extra_state_keyset(*desc->plugin)) {}

  plugin_state& gui_state() { return _gui_state; }
  extra_state& extra_state() { return _extra_state; }
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }

  void gui_changing(int index, plain_value plain) override;
  void gui_end_changes(int index) override { endEdit(gui_state().desc().param_mappings.index_to_tag[index]); }
  void gui_begin_changes(int index) override { beginEdit(gui_state().desc().param_mappings.index_to_tag[index]); }

  Steinberg::tresult PLUGIN_API getMidiControllerAssignment(
    Steinberg::int32 bus, Steinberg::int16 channel,
    Steinberg::Vst::CtrlNumber number, Steinberg::Vst::ParamID& id) override;

  Steinberg::IPlugView* PLUGIN_API createView(char const* name) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
};

}
