#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/components.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

// label that resizes to text content
class autofit_label:
public juce::Label,
public autofit_component
{
public:
  void textWasChanged() override final;
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
};

// button that resizes to text content
class autofit_button :
public juce::TextButton,
public autofit_component
{
public:
  autofit_button(lnf* lnf, std::string const& text);
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
};

// fixed size checkbox
class autofit_togglebutton :
public juce::ToggleButton,
public autofit_component
{
public:
  autofit_togglebutton() { setSize(24, 24); }
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
};

// dropdown that resizes to largest item
class autofit_combobox :
public juce::ComboBox,
public autofit_component
{
  lnf* const _lnf;
  bool const _autofit;
public:
  void autofit();
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
  autofit_combobox(lnf* lnf, bool autofit) : _lnf(lnf), _autofit(autofit) {}
};

// binding_component that is additionally bound to a single parameter value
// i.e., edit control or a label that displays a plugin parameter value
class param_component:
public binding_component
{
protected:
  param_desc const* const _param;

public:
  void state_changed(int index, plain_value plain) override;
  virtual ~param_component() { _gui->gui_state()->remove_listener(_param->info.global, this); }

protected:
  void init() override;
  virtual void own_param_changed(plain_value plain) = 0;
  param_component(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

// parameter name display
class param_name_label:
public binding_component,
public autofit_label
{
public:
  param_name_label(plugin_gui* gui, module_desc const* module, param_desc const* param):
  binding_component(gui, module, &param->param->gui.bindings, param->info.slot), autofit_label()
  { setText(param->info.name, juce::dontSendNotification); init(); }
};

// parameter value or name+value display
class param_value_label:
public param_component, 
public autofit_label
{
  bool const _both;
protected:
  void own_param_changed(plain_value plain) override final;
public:
  param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool both):
  param_component(gui, module, param), autofit_label(), _both(both) { init(); }
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
  { setText(_last_parsed = _param->param->domain.plain_to_text(false, plain), false); }

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
public autofit_togglebutton,
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
  void stoppedDragging() override { _gui->gui_end_changes(_param->info.global); }
  void startedDragging() override { _gui->gui_begin_changes(_param->info.global); }
  void valueChanged() override { _gui->gui_changing(_param->info.global, _param->param->domain.raw_to_plain(getValue())); }
};

// dropdown bound to single parameter
class param_combobox :
public param_component,
public autofit_combobox, 
public juce::ComboBox::Listener
{
protected:
  void own_param_changed(plain_value plain) override final
  { setSelectedItemIndex(plain.step() - _param->param->domain.min); }

public:
  ~param_combobox() { removeListener(this); }
  param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);
  void comboBoxChanged(ComboBox*) override final
  { _gui->gui_changed(_param->info.global, _param->param->domain.raw_to_plain(getSelectedItemIndex() + _param->param->domain.min)); }
};

}