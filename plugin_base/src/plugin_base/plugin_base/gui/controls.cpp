#include <plugin_base/gui/controls.hpp>

using namespace juce;

namespace plugin_base {

param_component::
param_component(plugin_gui* gui, module_desc const* module, param_desc const* param) :
binding_component(gui, module, &param->param->gui.bindings, param->info.slot), _param(param)
{ _gui->gui_state()->add_listener(_param->info.global, this); }

void
param_component::state_changed(int index, plain_value plain)
{
  if (index == _param->info.global)
    own_param_changed(plain);
  else
    binding_component::state_changed(index, plain);
}

void
param_component::init()
{
  // Must be called by subclass constructor as we dynamic_cast to Component inside.
  state_changed(_param->info.global, _gui->gui_state()->get_plain_at_index(_param->info.global));
  binding_component::init();
}

void
param_textbox::textEditorTextChanged(TextEditor&)
{
  plain_value plain;
  std::string text(getText().toStdString());
  if (!_param->param->domain.text_to_plain(false, text, plain)) return;
  _last_parsed = text;
  _gui->gui_changed(_param->info.global, plain);
}

param_value_label::
param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool both) :
param_component(gui, module, param), Label(), _both(both) 
{
  for(int d = 0; d < param->param->dependency_indices.size(); d++)
    _global_dependency_indices.push_back(gui->gui_state()->desc().mappings.topo_to_index
      [module->info.topo][module->info.slot][param->param->dependency_indices[d]][param->info.slot]);
  for(int d = 0; d < _global_dependency_indices.size(); d++)
    _gui->gui_state()->add_listener(_global_dependency_indices[d], this);
  init();
}

param_value_label::
~param_value_label()
{
  for (int d = 0; d < _global_dependency_indices.size(); d++)
    _gui->gui_state()->remove_listener(_global_dependency_indices[d], this);
}

void
param_value_label::own_param_changed(plain_value plain)
{ 
  std::string text = _gui->gui_state()->plain_to_text_at_index(false, _param->info.global, plain);
  if(_both) text = _param->info.name + " " + text;
  setText(text, dontSendNotification); 
}

void
param_value_label::state_changed(int index, plain_value plain)
{
  if (std::find(
    _global_dependency_indices.begin(), 
    _global_dependency_indices.end(), index) != _global_dependency_indices.end())
    own_param_changed(_gui->gui_state()->get_plain_at_index(_param->info.global));
  else
    param_component::state_changed(index, plain);
}

void 
param_toggle_button::own_param_changed(plain_value plain)
{
  _checked = plain.step() != 0;
  setToggleState(plain.step() != 0, dontSendNotification);
}

void 
param_toggle_button::buttonStateChanged(Button*)
{ 
  if(_checked == getToggleState()) return;
  plain_value plain = _param->param->domain.raw_to_plain(getToggleState() ? 1 : 0);
  _checked = getToggleState();
  _gui->gui_changed(_param->info.global, plain);
}

param_textbox::
param_textbox(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_component(gui, module, param), TextEditor()
{
  addListener(this);
  init();
}

param_toggle_button::
param_toggle_button(plugin_gui* gui, module_desc const* module, param_desc const* param):
param_component(gui, module, param), ToggleButton()
{ 
  auto value = param->param->domain.default_plain();
  _checked = value.step() != 0;
  addListener(this);
  init();
}

param_combobox::
param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_component(gui, module, param), ComboBox()
{
  auto const& domain = param->param->domain;
  for(int i = domain.min; i <= domain.max; i++)
    addItem(domain.raw_to_text(false, i), i - domain.min + 1);
  addListener(this);
  setEditableText(false);
  init();
}

param_slider::
param_slider(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_component(gui, module, param), Slider()
{
  switch (param->param->gui.edit_type)
  {
  case gui_edit_type::knob: setSliderStyle(Slider::RotaryVerticalDrag); break;
  case gui_edit_type::vslider: setSliderStyle(Slider::LinearVertical); break;
  case gui_edit_type::hslider: setSliderStyle(Slider::LinearHorizontal); break;
  default: assert(false); break;
  }
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  if (!param->param->domain.is_real()) setRange(param->param->domain.min, param->param->domain.max, 1);
  else setNormalisableRange(
    NormalisableRange<double>(param->param->domain.min, param->param->domain.max,
    [this](double s, double e, double v) { return _param->param->domain.normalized_to_raw(normalized_value(v)); },
    [this](double s, double e, double v) { return _param->param->domain.raw_to_normalized(v).value(); }));
  setDoubleClickReturnValue(true, param->param->domain.default_raw(), ModifierKeys::noModifiers);
  param_component::init();
}

param_dependent::
param_dependent(plugin_gui* gui, module_desc const* module, param_desc const* param):
param_component(gui, module, param), Component()
{
  assert(param->param->dependency_indices.size() > 0);
  for (int d = 0; d < param->param->dependency_indices.size(); d++)
    _global_dependency_indices.push_back(gui->gui_state()->desc().mappings.topo_to_index
      [module->info.topo][module->info.slot][param->param->dependency_indices[d]][param->info.slot]);
  for (int d = 0; d < _global_dependency_indices.size(); d++)
    gui->gui_state()->add_listener(_global_dependency_indices[d], this);

  _editor.addListener(this);
  init();
  update_editor();
}

param_dependent::
~param_dependent() 
{ 
  _editor.removeListener(this);
  for (int d = 0; d < _global_dependency_indices.size(); d++)
    _gui->gui_state()->remove_listener(_global_dependency_indices[d], this);
}

void 
param_dependent::state_changed(int index, plain_value plain)
{
  if (std::find(
    _global_dependency_indices.begin(),
    _global_dependency_indices.end(), index) != _global_dependency_indices.end())
    update_editor();
  else
    param_component::state_changed(index, plain);
}

void
param_dependent::comboBoxChanged(ComboBox* box)
{
  int value = _editor.getSelectedItemIndex();
  plain_value plain = _param->param->domain.raw_to_plain(value);
  auto clamped = _gui->gui_state()->clamp_dependent_at_index(_param->info.global, plain);
  _gui->gui_changed(_param->info.global, clamped);
}

void
param_dependent::update_editor_value()
{
  plain_value plain = _gui->gui_state()->get_plain_at_index(_param->info.global);
  plain_value clamped = _gui->gui_state()->clamp_dependent_at_index(_param->info.global, plain);
  _editor.setSelectedItemIndex(clamped.step(), dontSendNotification);
}

void
param_dependent::update_editor()
{
  _editor.removeListener(this);
  auto const& domain = *_gui->gui_state()->dependent_domain_at_index(_param->info.global);
  assert(domain.min == 0);
  _editor.clear(dontSendNotification);
  for (int i = 0; i <= domain.max; i++)
    _editor.addItem(_gui->gui_state()->raw_to_text_at_index(false, _param->info.global, i), i + 1);
  _editor.setEnabled(domain.max > 0);
  update_editor_value();
  _editor.addListener(this);
}

}