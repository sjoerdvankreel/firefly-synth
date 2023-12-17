#pragma once

#include <plugin_base/gui/gui.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/utility.hpp>

#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <map>
#include <utility>

namespace plugin_base::vst3 {

class pb_editor;

class pb_controller final:
public format_config,
public any_state_listener,
public gui_param_listener,
public Steinberg::Vst::IMidiMapping,
public Steinberg::Vst::EditControllerEx1
{
  // needs to be first, everyone else needs it
  std::unique_ptr<plugin_desc> _desc;
  pb_editor* _editor = {};
  plugin_state _gui_state = {};
  extra_state _extra_state;
  std::map<int, int> _midi_id_to_param = {};

  void param_state_changed(int index, plain_value plain);

public: 
  pb_controller(plugin_topo const* topo);
  ~pb_controller() { _gui_state.remove_any_listener(this); }

  OBJ_METHODS(pb_controller, EditControllerEx1)
  DEFINE_INTERFACES
    DEF_INTERFACE(IMidiMapping)
  END_DEFINE_INTERFACES(EditControllerEx1)
  REFCOUNT_METHODS(EditControllerEx1)
  PB_PREVENT_ACCIDENTAL_COPY(pb_controller);

  plugin_state& gui_state() { return _gui_state; }
  extra_state& extra_state() { return _extra_state; }
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }

  void gui_param_end_changes(int index) override;
  void gui_param_begin_changes(int index) override;
  void any_state_changed(int index, plain_value plain) override { param_state_changed(index, plain); }
  void gui_param_changing(int index, plain_value plain) override { param_state_changed(index, plain); }

  std::unique_ptr<host_menu> context_menu(int param_id) const override;
  std::filesystem::path resources_folder(std::filesystem::path const& binary_path) const override
  { return binary_path.parent_path().parent_path() / "Resources"; }

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
