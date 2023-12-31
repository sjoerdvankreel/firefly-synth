#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/components.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

// same as juce version but does not react to right-click
class text_button:
public juce::TextButton
{
public:
  void mouseUp(juce::MouseEvent const& e) override;
};

// same as juce version but does not react to right-click
class toggle_button :
public juce::ToggleButton
{
public:
  void mouseUp(juce::MouseEvent const& e) override;
};

// same as juce version but loads from resources folder
class image_component :
public juce::ImageComponent
{
public:
  image_component(
  format_config const* config, 
    std::string const& file_name, 
    juce::RectanglePlacement placement);
};

// button that resizes to text content
class autofit_button :
public text_button,
public autofit_component
{
public:
  autofit_button(lnf* lnf, std::string const& text);
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
};

// fixed size checkbox
class autofit_togglebutton :
public toggle_button,
public autofit_component
{
public:
  autofit_togglebutton() { setSize(21, 20); } // empirically determined
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
};

// label that resizes to text content
class autofit_label :
  public juce::Label,
  public autofit_component
{
  bool const _bold;
  int const _font_height;
public:
  bool bold() const { return _bold; }
  int font_height() const { return _font_height; }
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return getHeight(); }
  autofit_label(lnf* lnf, std::string const& reference_text, bool bold = false, int font_height = -1);
};

// dropdown that resizes to largest item
class autofit_combobox :
public juce::ComboBox,
public autofit_component
{
  lnf* const _lnf;
  bool const _autofit;
  float max_text_width(juce::PopupMenu const& menu);
public:
  void autofit();
  int fixed_width() const override { return getWidth(); }
  int fixed_height() const override { return _lnf->combo_height(); }
  autofit_combobox(lnf* lnf, bool autofit) : _lnf(lnf), _autofit(autofit) {}
};

// button that opens a popupmenu
// basically a combobox that shows a fixed button text
class menu_button :
public text_button
{
  int _selected_index = 0;
  std::vector<std::string> _items;
protected:
  void clicked() override;
public:
  std::function<void(int)> selected_index_changed;
  std::vector<std::string> const& get_items() const { return _items; }
  void set_items(std::vector<std::string> const& items) { _items = items; }
  void set_selected_index(int index) { _selected_index = std::clamp(index, 0, (int)_items.size() - 1); }
};

// tracks last parameter change
class last_tweaked_label :
public juce::Label,
public any_state_listener
{
  plugin_state const* const _state;
public:
  last_tweaked_label(plugin_state const* state);
  ~last_tweaked_label() { _state->remove_any_listener(this); }
  void any_state_changed(int index, plain_value plain) override;
};

// tracks last parameter change
class last_tweaked_editor :
public juce::TextEditor,
public juce::TextEditor::Listener,
public any_state_listener
{
  bool _updating = false;
  int _last_tweaked = -1;
  plugin_state* const _state;
public:
  ~last_tweaked_editor();
  last_tweaked_editor(plugin_state* state, lnf* lnf);

  void textEditorTextChanged(juce::TextEditor&) override;
  void any_state_changed(int index, plain_value plain) override;
};

// binds factory preset to extra_state
class preset_button:
public menu_button,
public extra_state_listener
{
  plugin_gui* const _gui;
  std::vector<factory_preset> _presets = {};
public:
  preset_button(plugin_gui* gui);
  void extra_state_changed() override;
  ~preset_button() { _gui->extra_state()->remove_listener(factory_preset_key, this); }
};

// binding_component that is additionally bound to a single parameter value
// i.e., edit control or a label that displays a plugin parameter value
// also provides host context menu
class param_component:
public binding_component,
public juce::MouseListener
{
protected:
  param_desc const* const _param;

public:
  void mouseUp(juce::MouseEvent const& e) override;
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
  static std::string label_ref_text(param_desc const* param, bool short_);
public:
  param_name_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool short_, lnf* lnf):
  binding_component(gui, module, &param->param->gui.bindings, param->info.slot), autofit_label(lnf, label_ref_text(param, short_))
  { setText(short_ ? param->param->info.tag.short_name : param->info.name, juce::dontSendNotification); init(); }
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
  param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool both, lnf* lnf);
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
  void stoppedDragging() override { _gui->param_end_changes(_param->info.global); }
  void startedDragging() override { _gui->param_begin_changes(_param->info.global); }
  void valueChanged() override { _gui->param_changing(_param->info.global, _param->param->domain.raw_to_plain(getValue())); }
  juce::String getTextFromValue(double value) override 
  { return juce::String(_param->info.name + ": ") + juce::Slider::getTextFromValue(value * (_param->param->domain.display == domain_display::percentage ? 100 : 1)); }
};

// dropdown bound to single parameter
class param_combobox :
public param_component,
public autofit_combobox, 
public juce::ComboBox::Listener
{
protected:
  void own_param_changed(plain_value plain) override final;

public:
  ~param_combobox() { removeListener(this); }
  param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);

  void showPopup() override;
  void comboBoxChanged(ComboBox*) override final
  { _gui->param_changed(_param->info.global, _param->param->domain.raw_to_plain(getSelectedId() - 1 + _param->param->domain.min)); }
};

}