#include <plugin_base/gui/controls.hpp>

using namespace juce;

namespace plugin_base {

param_component::
param_component(plugin_gui* gui, module_desc const* module, param_desc const* param) :
binding_component(gui, module, &param->param->gui.bindings, param->info.slot), _param(param)
{ _gui->add_plugin_listener(_param->info.global, this); }

void
param_component::plugin_changed(int index, plain_value plain)
{
  if (index == _param->info.global)
    own_param_changed(plain);
  else
    binding_component::plugin_changed(index, plain);
}

void
param_component::init()
{
  // Must be called by subclass constructor as we dynamic_cast to Component inside.
  auto const& own_mapping = _gui->desc()->mappings.params[_param->info.global];
  plugin_changed(_param->info.global, own_mapping.value_at(_gui->gui_state()));
  binding_component::init();
}

void
param_textbox::textEditorTextChanged(TextEditor&)
{
  plain_value plain;
  std::string text(getText().toStdString());
  if (!_param->param->domain.text_to_plain(text, plain)) return;
  _last_parsed = text;
  _gui->gui_changed(_param->info.global, plain);
}

void
param_value_label::own_param_changed(plain_value plain)
{ 
  std::string text = _param->param->domain.plain_to_text(plain);
  if(_both) text = _param->info.name + " " + text;
  setText(text, dontSendNotification); 
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
    addItem(domain.raw_to_text(i), i - domain.min + 1);
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
  auto const& topo_to_index = _gui->desc()->mappings.topo_to_index;
  auto const& param_indices = topo_to_index[_module->info.topo][_module->info.slot];
  _dependent_global_index = param_indices[param->param->dependent_index][param->info.slot];
  for (int i = 0; i < param->param->dependents.size(); i++)
  {
    auto const& domain = param->param->dependents[i];
    auto& dependent = _dependents.emplace_back(std::make_unique<ComboBox>());
    for (int j = domain.min; j <= domain.max; j++)
      dependent->addItem(domain.raw_to_text(j), j - domain.min + 1);
    addChildComponent(dependent.get());
  }
  init();
  update_dependents();
  gui->add_plugin_listener(_dependent_global_index, this);
}

void
param_dependent::resized()
{
  for(int i = 0; i < _dependents.size(); i++)
    _dependents[i]->setBounds(getLocalBounds());
}

void 
param_dependent::plugin_changed(int index, plain_value plain)
{
  if(index == _dependent_global_index)
    update_dependents();
  else
    param_component::plugin_changed(index, plain);
}

void
param_dependent::update_dependents()
{
  auto const& param_state = _gui->gui_state()[_module->info.topo][_module->info.slot];
  int dependent_value = param_state[_param->param->dependent_index][_param->info.slot].step();
  if(_dependent_value == dependent_value) return;

  _dependent_value = dependent_value;
  int dependent_min = _module->module->params[_param->param->dependent_index].domain.min;
  for(int i = 0; i < _dependents.size(); i++)
  {
    auto const& domain = _param->param->dependents[i];
    _dependents[i]->setSelectedItemIndex(0);
    _dependents[i]->setEnabled(domain.max - domain.min > 0);
    _dependents[i]->setVisible(i == dependent_value - dependent_min);
  }
}

}