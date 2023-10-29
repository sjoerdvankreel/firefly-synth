#include <plugin_base/gui/controls.hpp>
#include <algorithm>

using namespace juce;

namespace plugin_base {

static void
fill_popup_menu(param_domain const& domain, PopupMenu& menu, gui_submenu const* data)
{
  menu.clear();
  for (int i = 0; i < data->indices.size(); i++)
    menu.addItem(data->indices[i] + 1, domain.raw_to_text(false, data->indices[i]));
  for(int i = 0; i < data->children.size(); i++)
  {
    PopupMenu child;
    fill_popup_menu(domain, child, data->children[i].get());
    menu.addSubMenu(data->children[i]->name, child);
  }
}

void
autofit_label::textWasChanged()
{
  auto border_size = getBorderSize();
  auto const& label_font = getLookAndFeel().getLabelFont(*this);
  float th = label_font.getHeight();
  float tw = label_font.getStringWidthFloat(getText());
  float nw = std::ceil(tw) + border_size.getLeftAndRight();
  if(getHeight() > 0)
    setSize(nw, getHeight());
  else
    setSize(nw, std::ceil(th) + border_size.getTopAndBottom());
}

autofit_button::
autofit_button(lnf* lnf, std::string const& text)
{
  float vmargin = 6.0f;
  float hmargin = 16.0f;
  setButtonText(text);
  auto const& button_font = lnf->getTextButtonFont(*this, getHeight());
  float th = button_font.getHeight();
  float tw = button_font.getStringWidthFloat(getButtonText());
  setSize(std::ceil(tw) + hmargin, std::ceil(th) + vmargin);
}

void
autofit_combobox::autofit()
{
  if(!_autofit) return;

  float max_width = 0;  
  int const hpadding = 22;
  int count = getNumItems();
  auto const& font = _lnf->getComboBoxFont(*this);
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

void
param_value_label::own_param_changed(plain_value plain)
{ 
  std::string text = _gui->gui_state()->plain_to_text_at_index(false, _param->info.global, plain);
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
param_component(gui, module, param), autofit_togglebutton()
{ 
  auto value = param->param->domain.default_plain();
  _checked = value.step() != 0;
  addListener(this);
  init();
}

param_combobox::
param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf) :
param_component(gui, module, param), 
autofit_combobox(lnf, param->param->gui.edit_type == gui_edit_type::autofit_list)
{
  auto const& domain = param->param->domain;
  auto const& param_gui = param->param->gui;
  if(!param_gui.submenu)
    for(int i = 0; i <= domain.max; i++)
      addItem(domain.raw_to_text(false, i), i + 1);
  else
  {
    assert(param_gui.edit_type == gui_edit_type::list);
    fill_popup_menu(domain, *getRootMenu(), param_gui.submenu.get());
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
  auto contents = param->param->gui.label.contents;
  switch (contents)
  {
  case gui_label_contents::none:
  case gui_label_contents::name:
    setPopupDisplayEnabled(true, true, nullptr);
    break;
  case gui_label_contents::both:
  case gui_label_contents::value:
    break;
  default:
    assert(false);
    break;
  }
  
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

}