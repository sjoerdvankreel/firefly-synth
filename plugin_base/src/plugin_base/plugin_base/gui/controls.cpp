#include <plugin_base/gui/controls.hpp>
#include <algorithm>

using namespace juce;

namespace plugin_base {

void
autofit_label::textWasChanged()
{
  auto border_size = getBorderSize();
  float th = getFont().getHeight();
  float tw = getFont().getStringWidthFloat(getText());
  float nw = std::ceil(tw) + border_size.getLeftAndRight();
  if(getHeight() > 0)
    setSize(nw, getHeight());
  else
    setSize(nw, std::ceil(th) + border_size.getTopAndBottom());
}

void
autofit_combobox::autofit()
{
  if(!_autofit) return;

  float max_width = 0;  
  int const hpadding = 42;
  int count = getNumItems();
  auto const& font = getLookAndFeel().getComboBoxFont(*this);
  float text_height = font.getHeight();
  for (int i = 0; i < count; i++)
  {
    auto text = getItemText(i);
    auto text_width = font.getStringWidthFloat(text);
    max_width = std::max(max_width, text_width);
  }
  setSize(std::ceil(max_width + hpadding), std::ceil(text_height));
}

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
param_component(gui, module, param), autofit_label(), _both(both)
{
  for (int d = 0; d < param->param->dependent.dependencies.size(); d++)
    _global_dependencies.push_back(gui->gui_state()->desc().mappings.topo_to_index
      [module->info.topo][module->info.slot][param->param->dependent.dependencies[d]][param->info.slot]);
  for (int d = 0; d < _global_dependencies.size(); d++)
    _gui->gui_state()->add_listener(_global_dependencies[d], this);
  init();
}

param_value_label::
~param_value_label()
{
  for (int d = 0; d < _global_dependencies.size(); d++)
    _gui->gui_state()->remove_listener(_global_dependencies[d], this);
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
    _global_dependencies.begin(),
    _global_dependencies.end(), index) != _global_dependencies.end())
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
param_component(gui, module, param), autofit_togglebutton()
{ 
  auto value = param->param->domain.default_plain();
  _checked = value.step() != 0;
  addListener(this);
  init();
}

param_combobox::
param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_component(gui, module, param), 
autofit_combobox(param->param->gui.edit_type == gui_edit_type::autofit_list)
{
  auto const& domain = param->param->domain;
  auto const& param_gui = param->param->gui;
  if(!param_gui.submenus.size())
    for(int i = 0; i <= domain.max; i++)
      addItem(domain.raw_to_text(false, i), i + 1);
  else
  {
    assert(param_gui.edit_type == gui_edit_type::list);
    for (int i = 0; i < param_gui.submenus.size(); i++)
    {
      PopupMenu submenu;
      for(int j = 0; j < param_gui.submenus[i].indices.size(); j++)
      {
        int index = param_gui.submenus[i].indices[j];
        submenu.addItem(index + 1, domain.raw_to_text(false, index));
      }
      getRootMenu()->addSubMenu(param_gui.submenus[i].name, submenu);
    }
  }
  autofit();
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
  assert(param->param->dependent.dependencies.size() > 0);
  for (int d = 0; d < param->param->dependent.dependencies.size(); d++)
    _global_dependencies.push_back(gui->gui_state()->desc().mappings.topo_to_index
      [module->info.topo][module->info.slot][param->param->dependent.dependencies[d]][param->info.slot]);
  for (int d = 0; d < _global_dependencies.size(); d++)
    gui->gui_state()->add_listener(_global_dependencies[d], this);

  for (int i = 0; i < param->param->dependent.domains.size(); i++)
  {
    auto const& domain = param->param->dependent.domains[i];
    auto& editor = _editors.emplace_back(std::make_unique<ComboBox>());
    for (int j = domain.min; j <= domain.max; j++)
      editor->addItem(domain.raw_to_text(false, j), j - domain.min + 1);
    editor->setSelectedItemIndex(0, dontSendNotification);
    addChildComponent(editor.get());
    editor->addListener(this);
  }

  init();
  update_editors();
}

param_dependent::
~param_dependent() 
{ 
  for (int i = 0; i < _editors.size(); i++)
    _editors[i]->removeListener(this);
  for (int d = 0; d < _global_dependencies.size(); d++)
    _gui->gui_state()->remove_listener(_global_dependencies[d], this);
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
  if (std::find(
    _global_dependencies.begin(),
    _global_dependencies.end(), index) != _global_dependencies.end())
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
param_dependent::own_param_changed(plain_value plain)
{
  int domain_index = _gui->gui_state()->dependent_domain_index_at_index(_param->info.global);
  _editors[domain_index]->setSelectedItemIndex(plain.step(), juce::dontSendNotification);
}

void
param_dependent::update_editors()
{
  int domain_index = _gui->gui_state()->dependent_domain_index_at_index(_param->info.global);
  for (int i = 0; i < _editors.size(); i++)
  {
    _editors[i]->setVisible(domain_index == i);
    _editors[i]->setEnabled(_param->param->dependent.domains[i].max > 0);
  }
}

}