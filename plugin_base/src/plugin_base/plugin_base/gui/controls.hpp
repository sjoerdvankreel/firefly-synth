#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/components.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

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
    format_basic_config const* config,
    std::string const& theme,
    std::string const& file_name, 
    juce::RectanglePlacement placement);
};

// fixed size checkbox
class autofit_togglebutton :
public toggle_button,
public autofit_component
{
  lnf* const _lnf;
  bool const _tabular;
public:
  int fixed_width(int parent_w, int parent_h) const override { return _lnf->toggle_height(_tabular); }
  int fixed_height(int parent_w, int parent_h) const override { return _lnf->toggle_height(_tabular); }
  autofit_togglebutton(lnf* lnf, bool tabular): _lnf(lnf), _tabular(tabular) { setSize(lnf->toggle_height(_tabular), lnf->toggle_height(_tabular)); }
};

// label that resizes to text content
class autofit_label :
  public juce::Label,
  public autofit_component
{
  bool const _bold;
  bool const _tabular;
  int const _font_height;
public:
  bool bold() const { return _bold; }
  bool tabular() const { return _tabular; }
  int font_height() const { return _font_height; }
  int fixed_width(int parent_w, int parent_h) const override { return getWidth(); }
  int fixed_height(int parent_w, int parent_h) const override { return getHeight(); }
  autofit_label(lnf* lnf, std::string const& reference_text, bool bold = false, int font_height = -1, bool tabular = false);
};

// dropdown that resizes to largest item
class autofit_combobox :
public juce::ComboBox,
public autofit_component
{
  lnf* const _lnf;
  bool const _autofit;
  bool const _tabular;
  float max_text_width(juce::PopupMenu const& menu);
public:
  void autofit();
  autofit_combobox(lnf* lnf, bool autofit, bool tabular) : _lnf(lnf), _autofit(autofit), _tabular(tabular) {}
  int fixed_width(int parent_w, int parent_h) const override { return getWidth(); }
  int fixed_height(int parent_w, int parent_h) const override { return _lnf->combo_height(_tabular); }
};

// tracks last parameter change
class last_tweaked_label :
public juce::Label,
public any_state_listener
{
  plugin_gui* const _gui;
public:
  last_tweaked_label(plugin_gui* gui, lnf* lnf);
  ~last_tweaked_label() { _gui->automation_state()->remove_any_listener(this); }
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

// binds theme to global user config
class theme_combo:
public autofit_combobox
{
  plugin_gui* const _gui;
  std::vector<std::string> _themes = {};
public:
  theme_combo(plugin_gui* gui, lnf* lnf);
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
  param_desc const* param() const { return _param; }
  void mouseUp(juce::MouseEvent const& e) override;
  void state_changed(int index, plain_value plain) override;
  virtual ~param_component() { _gui->automation_state()->remove_listener(_param->info.global, this); }

protected:
  void init() override;
  virtual void own_param_changed(plain_value plain) = 0;
  param_component(plugin_gui* gui, module_desc const* module, param_desc const* param);
};

// just a drag handle for d&d support
class param_drag_label:
public binding_component,
public autofit_component,
public juce::Component
{
  static int const _size = 7;
  lnf* const _lnf;
  param_desc const* const _param;

protected:
  void enablementChanged() { repaint(); }

public:
  void paint(juce::Graphics& g) override;
  juce::MouseCursor getMouseCursor() override;
  void mouseDrag(juce::MouseEvent const& e) override;
  int fixed_width(int parent_w, int parent_h) const override { return _size; }
  int fixed_height(int parent_w, int parent_h) const override { return _size; }
  param_drag_label(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);
};

// static parameter name display + d&d support
class param_name_label:
public binding_component,
public autofit_label
{
  param_desc const* const _param;
  param_desc const* const _alternate_drag_param;
  output_desc const* const _alternate_drag_output;
  static std::string label_ref_text(param_desc const* param);
public:
  juce::MouseCursor getMouseCursor() override;
  void mouseDrag(juce::MouseEvent const& e) override;
  param_name_label(
    plugin_gui* gui, module_desc const* module, param_desc const* param, 
    param_desc const* alternate_drag_param, output_desc const* alternate_drag_output, lnf* lnf);
};

// dynamic parameter value display + d&d support
class param_value_label:
public param_component, 
public autofit_label
{
  static std::string value_ref_text(plugin_gui* gui, param_desc const* param);
protected:
  void own_param_changed(plain_value plain) override final;
public:
  juce::MouseCursor getMouseCursor() override;
  void mouseDrag(juce::MouseEvent const& e) override;
  param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);
};

// dynamic module name display
class module_name_label :
public param_component,
public autofit_label
{
protected:
  void own_param_changed(plain_value plain) override final;
public:
  module_name_label(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);
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
  param_toggle_button(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);
};

// slider bound to single parameter
// only horizontally autofits knobs not sliders!
class param_slider:
public param_component,
public juce::Slider,
public autofit_component,
public modulation_output_listener
{
  float _min_modulation_output = -1.0f;
  float _max_modulation_output = -1.0f;
  double _modulation_output_activated_time_seconds = {};

protected:
  void own_param_changed(plain_value plain) override final
  { setValue(_param->param->domain.plain_to_raw(plain), juce::dontSendNotification); }

public: 
  ~param_slider();
  param_slider(plugin_gui* gui, module_desc const* module, param_desc const* param);

  // param modulation outputs
  float min_modulation_output() const { return _min_modulation_output; }
  float max_modulation_output() const { return _max_modulation_output; }
  void modulation_outputs_changed(std::vector<modulation_output> const& outputs) override;
  double modulation_output_activated_time_seconds() const { return _modulation_output_activated_time_seconds; }

  int fixed_width(int parent_w, int parent_h) const override;
  int fixed_height(int parent_w, int parent_h) const override { return -1; }

  void stoppedDragging() override { _gui->param_end_changes(_param->info.global); }
  void startedDragging() override { _gui->param_begin_changes(_param->info.global); }
  void valueChanged() override { _gui->param_changing(_param->info.global, _param->param->domain.raw_to_plain(getValue())); }

  juce::String getTextFromValue(double value) override 
  { return juce::String(_param->info.name + ": ") + juce::Slider::getTextFromValue(value * (_param->param->domain.display == domain_display::percentage ? 100 : 1)); }
};

enum class drop_target_action { none, never, not_now, ok };

// dropdown bound to single parameter
// NOTE: we only support drag-drop onto combos for now
class param_combobox :
public param_component,
public autofit_combobox, 
public juce::DragAndDropTarget,
public juce::ComboBox::Listener
{
  drop_target_action _drop_target_action = drop_target_action::none;
  int get_item_tag(std::string const& item_id) const;

  void update_all_items_enabled_state();

protected:
  void own_param_changed(plain_value plain) override final;

public:
  ~param_combobox() { removeListener(this); }
  param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf);

  void showPopup() override;
  void comboBoxChanged(ComboBox*) override final;

  // d&d support
  drop_target_action get_drop_target_action() const { return _drop_target_action; }
  void itemDropped(juce::DragAndDropTarget::SourceDetails const& details) override;
  void itemDragExit(juce::DragAndDropTarget::SourceDetails const& details) override;
  void itemDragEnter(juce::DragAndDropTarget::SourceDetails const& details) override;
  bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& details) override;
};

}