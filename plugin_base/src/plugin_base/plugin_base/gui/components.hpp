#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/listeners.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

// base class for anything that should react to gui_bindings
// i.e. has it's enabled/visible bound to a set of plugin parameters
class binding_component:
public plugin_listener
{
public:
  virtual ~binding_component();
  void plugin_changed(int index, plain_value plain) override;

protected:
  plugin_gui* const _gui;
  int const _own_slot_index;
  module_desc const* const _module;
  gui_bindings const* const _bindings;

  virtual void init();
  binding_component(
    plugin_gui* gui, module_desc const* module, 
    gui_bindings const* bindings, int own_slot_index);

private:
  std::vector<int> _enabled_values = {};
  std::vector<int> _enabled_params = {};
  std::vector<int> _visibility_values = {};
  std::vector<int> _visibility_params = {};

  void setup_bindings(std::vector<int> const& topo_params, std::vector<int>& params);
  bool bind(gui_binding const& binding, std::vector<int> const& params, std::vector<int>& values);
};

// resizes single child on resize
class group_component :
public juce::GroupComponent {
public:
  void resized() override;
};

// grid component as opposed to grid layout
// resizes children on resize
class grid_component:
public juce::Component
{
  gui_dimension const _dimension;
  std::vector<gui_position> _positions = {};

public:
  void resized() override;
  void add(Component& child, gui_position const& position);
  void add(Component& child, bool vertical, int position) 
  { add(child, gui_position { vertical? position: 0, vertical? 0: position }); }

  // Can't intercept mouse as we may be invisible on top of 
  // another grid in case of param or section dependent visibility.
  grid_component(gui_dimension const& dimension) :
  _dimension(dimension) { setInterceptsMouseClicks(false, true); }
  grid_component(bool vertical, int count) :
  grid_component(gui_dimension { vertical ? count : 1, vertical ? 1 : count }) {}
};

// binding_component that hosts a number of plugin parameters
class section_grid_component :
public binding_component,
public grid_component
{
public:
  section_grid_component(plugin_gui* gui, module_desc const* module, section_topo const* section):
  binding_component(gui, module, &section->gui.bindings, 0), grid_component(section->gui.dimension) { init(); }
};

// binding_component that hosts a single section_grid_component
class section_group_component :
public binding_component,
public group_component
{
public:
  section_group_component(plugin_gui* gui, module_desc const* module, section_topo const* section):
  binding_component(gui, module, &section->gui.bindings, 0), group_component() { init(); }
};

// binding_component that is additionally bound to a single parameter value
// i.e., edit control or a label that displays a plugin parameter value
class param_component:
public binding_component
{
protected:
  param_desc const* const _param;

public:
  void plugin_changed(int index, plain_value plain) override final;
  virtual ~param_component() { _gui->remove_plugin_listener(_param->global, this); }

protected:
  void init() override final;
  virtual void own_param_changed(plain_value plain) = 0;
  param_component(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

// parameter name display
class param_name_label:
public binding_component,
public juce::Label
{
public:
  param_name_label(plugin_gui* gui, module_desc const* module, param_desc const* param):
  binding_component(gui, module, &param->param->gui.bindings, param->slot), Label()
  { setText(param->name, juce::dontSendNotification);  }
};

// parameter value or name+value display
class param_value_label:
public param_component, 
public juce::Label
{
  bool const _both;
protected:
  void own_param_changed(plain_value plain) override final;
public:
  param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool both):
  param_component(gui, module, param), Label(), _both(both) { init(); }
};

// textbox bound to single parameter
class param_textbox :
public param_component,
public juce::TextEditor, 
public juce::TextEditor::Listener
{
  std::string _last_parsed;
protected:
  void own_param_changed(plain_value plain) override final
  { setText(_last_parsed = _param->param->domain.plain_to_text(plain), false); }

public:
  void textEditorTextChanged(TextEditor&) override;
  void textEditorFocusLost(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorReturnKeyPressed(TextEditor&) override { setText(_last_parsed, false); }
  void textEditorEscapeKeyPressed(TextEditor&) override { setText(_last_parsed, false); }

  ~param_textbox() { removeListener(this); }
  param_textbox(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

// checkbox bound to single parameter
class param_toggle_button :
public param_component,
public juce::ToggleButton, 
public juce::Button::Listener
{
  bool _checked = false;
protected:
  void own_param_changed(plain_value plain) override final;

public:
  void buttonClicked(Button*) override {}
  void buttonStateChanged(Button*) override;
  ~param_toggle_button() { removeListener(this); }
  param_toggle_button(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

// slider bound to single parameter
class param_slider:
public param_component,
public juce::Slider
{
protected:
  void own_param_changed(plain_value plain) override final
  { setValue(_param->param->domain.plain_to_raw(plain), juce::dontSendNotification); }

public: 
  param_slider(plugin_gui* gui, module_desc const* module, param_desc const* param);
  void stoppedDragging() override { _gui->gui_end_changes(_param->global); }
  void startedDragging() override { _gui->gui_begin_changes(_param->global); }
  void valueChanged() override { _gui->gui_changing(_param->global, _param->param->domain.raw_to_plain(getValue())); }
};

// dropdown bound to single parameter
class param_combobox :
public param_component,
public juce::ComboBox, 
public juce::ComboBox::Listener
{
protected:
  void own_param_changed(plain_value plain) override final
  { setSelectedItemIndex(plain.step() - _param->param->domain.min); }

public:
  ~param_combobox() { removeListener(this); }
  param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param);
  void comboBoxChanged(ComboBox*) override final
  { _gui->gui_changed(_param->global, _param->param->domain.raw_to_plain(getSelectedItemIndex() + _param->param->domain.min)); }
};

}