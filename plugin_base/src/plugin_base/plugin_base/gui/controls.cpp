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

void
param_value_label::own_param_changed(plain_value plain)
{ 
  std::string text = _param->param->domain.plain_to_text(false, plain);
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
  _global_dependency_index = gui->gui_state()->desc().dependency_index(param->info.global);
  assert(_global_dependency_index != -1);
  for (int i = 0; i < param->param->dependent_domains.size(); i++)
  {
    auto const& domain = param->param->dependent_domains[i];
    auto& editor = _editors.emplace_back(std::make_unique<ComboBox>());
    for (int j = domain.min; j <= domain.max; j++)
      editor->addItem(domain.raw_to_text(false, j), j - domain.min + 1);
    editor->setSelectedItemIndex(0, dontSendNotification);
    addChildComponent(editor.get());
    editor->addListener(this);
  }
  init();
  update_editors();
  gui->gui_state()->add_listener(_global_dependency_index, this);
}

param_dependent::
~param_dependent() 
{ 
  _gui->gui_state()->remove_listener(_global_dependency_index, this);
  for(int i = 0; i < _editors.size(); i++)
    _editors[i]->removeListener(this);
}

void
param_dependent::resized()
{
  for(int i = 0; i < _editors.size(); i++)
    _editors[i]->setBounds(getLocalBounds());
}

void 
param_dependent::state_changed(int index, plain_value plain)
{
  if(index == _global_dependency_index)
    update_editors();
  else
    param_component::state_changed(index, plain);
}

void
param_dependent::comboBoxChanged(ComboBox* box)
{
  for(int i = 0; i < _editors.size(); i++)
  {
    if(_editors[i].get() != box) continue;
    int value = box->getSelectedItemIndex();
    plain_value plain = _param->param->domain.raw_to_plain(value);
    _gui->gui_changed(_param->info.global, plain);
    return;
  }
  assert(false);
}

void
param_dependent::update_editors()
{
  int dependency_value = _gui->gui_state()->get_plain_at_index(_global_dependency_index).step();
  for (int i = 0; i < _editors.size(); i++)
  {
    _editors[i]->setVisible(i == dependency_value);
    _editors[i]->setEnabled(_param->param->dependent_domains[i].max > 0);
  }
}

void
param_dependent::own_param_changed(plain_value plain)
{
  int dependency_value = _gui->gui_state()->get_plain_at_index(_global_dependency_index).step();
  int index = _param->param->clamp_dependent(dependency_value, plain).step();
  _editors[dependency_value]->setSelectedItemIndex(index, juce::dontSendNotification);
}

}