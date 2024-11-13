#pragma once

#include <plugin_base/gui/gui.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base.vst3/utility.hpp>

#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

#include <map>
#include <stack>
#include <utility>

namespace plugin_base::vst3 {

class pb_editor;

class pb_basic_config:
public format_basic_config
{
public:
  static pb_basic_config const* instance();
  std::string format_name() const override { return "VST3"; }
  std::filesystem::path resources_folder(std::filesystem::path const& binary_path) const override
  { return binary_path.parent_path().parent_path() / "Resources"; }
};

class pb_controller final:
public format_menu_handler,
public any_state_listener,
public gui_param_listener,
public Steinberg::Vst::IMidiMapping,
public Steinberg::Vst::EditControllerEx1
{
  // needs to be first, everyone else needs it
  std::unique_ptr<plugin_desc> _desc;
  pb_editor* _editor = {};
  plugin_state _automation_state = {};
  extra_state _extra_state;
  std::map<int, int> _midi_id_to_param = {};
  std::stack<int> _undo_tokens = {};
  std::vector<modulation_output> _modulation_outputs = {};

  // see param_state_changed and setParamNormalized
  // when host comes at us with an automation value, that is
  // reported through setParamNormalized, which through a
  // bunch of gui event handlers comes back to us at param_state_changed
  // which then AGAIN calls setParamNormalized and (even worse) performEdit
  // could maybe be fixed by differentiating between host-initiated and
  // user-initiated gui changes, but it's a lot of work, so just go with
  // a reentrancy flag
  bool _inside_set_param_normalized = false;

  void param_state_changed(int index, plain_value plain);

public: 
  ~pb_controller();
  pb_controller(plugin_topo const* topo);

  OBJ_METHODS(pb_controller, EditControllerEx1)
  DEFINE_INTERFACES
    DEF_INTERFACE(IMidiMapping)
  END_DEFINE_INTERFACES(EditControllerEx1)
  REFCOUNT_METHODS(EditControllerEx1)
  PB_PREVENT_ACCIDENTAL_COPY(pb_controller);

  extra_state& extra_state_() { return _extra_state; }
  plugin_state& automation_state() { return _automation_state; }
  void editorDestroyed(Steinberg::Vst::EditorView*) override { _editor = nullptr; }

  std::unique_ptr<host_menu> context_menu(int param_id) const override;

  void gui_param_end_changes(int index) override;
  void gui_param_begin_changes(int index) override;
  void any_state_changed(int index, plain_value plain) override { param_state_changed(index, plain); }
  void gui_param_changing(int index, plain_value plain) override { param_state_changed(index, plain); }

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
