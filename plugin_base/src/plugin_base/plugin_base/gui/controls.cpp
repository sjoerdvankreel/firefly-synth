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

last_tweaked_label::
last_tweaked_label(plugin_state const* state, std::string const& prefix):
_state(state), _prefix(prefix)
{
  state->add_any_listener(this);
  any_state_changed(0, state->get_plain_at_index(0));
}

void 
last_tweaked_label::any_state_changed(int index, plain_value plain)
{
  if(_state->desc().params[index]->param->dsp.direction == param_direction::output) return;
  std::string label = _state->desc().params[index]->full_name;
  setText(String(_prefix + " " + label), dontSendNotification);
}

last_tweaked_editor::
last_tweaked_editor(plugin_state const* state) :
_state(state)
{
  state->add_any_listener(this);
  any_state_changed(0, state->get_plain_at_index(0));
}

void
last_tweaked_editor::any_state_changed(int index, plain_value plain)
{
  if (_state->desc().params[index]->param->dsp.direction == param_direction::output) return;
  setText(String(_state->desc().params[index]->param->domain.plain_to_text(false, plain)), dontSendNotification);
}

image_component::
image_component(
  format_config const* config, 
  std::string const& file_name, 
  RectanglePlacement placement)
{
  String path(get_resource_location(config, file_name).string());
  setImage(ImageCache::getFromFile(path), placement);
}

autofit_label::
autofit_label(lnf* lnf, std::string const& reference_text, bool bold, int height):
_bold(bold), _font_height(height)
{
  auto border_size = getBorderSize();
  auto label_font = lnf->getLabelFont(*this);
  if(bold) label_font = label_font.boldened();
  if(height != -1) label_font = label_font.withHeight(height);
  float th = label_font.getHeight();
  float tw = label_font.getStringWidthFloat(reference_text);
  float nw = std::ceil(tw) + border_size.getLeftAndRight();
  setSize(nw, std::ceil(th) + border_size.getTopAndBottom());
  setText(reference_text, dontSendNotification);
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

float 
autofit_combobox::max_text_width(PopupMenu const& menu)
{
  float result = 0;
  PopupMenu::MenuItemIterator iter(menu);
  auto const& font = _lnf->getComboBoxFont(*this);
  while(iter.next())
  {
    auto text = iter.getItem().text;
    auto text_width = font.getStringWidthFloat(text);
    if(iter.getItem().subMenu)
      result = std::max(result, max_text_width(*iter.getItem().subMenu));
    else
      result = std::max(result, text_width);
  }
  return result;
}

void
autofit_combobox::autofit()
{
  if(!_autofit) return;

  int const hpadding = 22;
  auto const& font = _lnf->getComboBoxFont(*this);
  float text_height = font.getHeight();
  float max_width = max_text_width(*getRootMenu());
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

// Just guess max value is representative of the longest text.
param_value_label::
param_value_label(plugin_gui* gui, module_desc const* module, param_desc const* param, bool both, lnf* lnf) :
param_component(gui, module, param), 
autofit_label(lnf, _gui->gui_state()->plain_to_text_at_index(false, 
  _param->info.global, param->param->domain.raw_to_plain(param->param->domain.max))), _both(both)
{ init(); }

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
  auto value = param->param->domain.default_plain(module->info.slot, param->info.slot);
  _checked = value.step() != 0;
  addListener(this);
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
  if(param->param->domain.unit.size())
    setTextValueSuffix(" " + param->param->domain.unit);
  setNumDecimalPlacesToDisplay(param->param->domain.precision);
  if (!param->param->domain.is_real()) setRange(param->param->domain.min, param->param->domain.max, 1);
  else setNormalisableRange(
    NormalisableRange<double>(param->param->domain.min, param->param->domain.max,
    [this](double s, double e, double v) { return _param->param->domain.normalized_to_raw(normalized_value(v)); },
    [this](double s, double e, double v) { return _param->param->domain.raw_to_normalized(v).value(); }));
  setDoubleClickReturnValue(true, param->param->domain.default_raw(_module->info.slot, _param->info.slot), ModifierKeys::noModifiers);
  param_component::init();
}

param_combobox::
param_combobox(plugin_gui* gui, module_desc const* module, param_desc const* param, lnf* lnf) :
param_component(gui, module, param), 
autofit_combobox(lnf, param->param->gui.edit_type == gui_edit_type::autofit_list)
{
  auto const& domain = param->param->domain;
  auto const& param_gui = param->param->gui;
  if(!param_gui.submenu)
  {
    int index = 0;
    for(int i = domain.min; i <= domain.max; i++, index++)
      addItem(domain.raw_to_text(false, i), index + 1);
  }
  else
    fill_popup_menu(domain, *getRootMenu(), param_gui.submenu.get());
  autofit();
  addListener(this);
  setEditableText(false);
  init();
}

void 
param_combobox::showPopup()
{
  auto const& items = _param->param->domain.items;
  if(_param->param->gui.item_enabled.is_param_bound())
  {
    auto m = _param->param->gui.item_enabled.param;
    if(m.param_slot == gui_item_binding::match_param_slot) m.param_slot = _param->info.slot;
    auto other = _gui->gui_state()->get_plain_at(m.module_index, m.module_slot, m.param_index, m.param_slot);
    for(int i = 0; i < items.size(); i++)
      setItemEnabled(i + 1, _param->param->gui.item_enabled.selector(other.step(), _param->param->domain.min + i));
  }
  else if (_param->param->gui.item_enabled.auto_bind)
  {
    auto const& topo = *_gui->gui_state()->desc().plugin;
    for (int i = 0; i < items.size(); i++)
    {
      bool enabled = true;
      auto const& that_param = items[i].param_topo;
      auto const& that_topo = topo.modules[that_param.module_index].params[that_param.param_index];
      if(that_topo.gui.bindings.enabled.slot_selector != nullptr)
        enabled &= that_topo.gui.bindings.enabled.slot_selector(that_param.module_index);
      if (that_topo.gui.bindings.visible.slot_selector != nullptr)
        enabled &= that_topo.gui.bindings.visible.slot_selector(that_param.module_index);
      if (that_topo.gui.bindings.enabled.param_selector != nullptr)
      {
        std::vector<int> that_values;
        for(int j = 0; j < that_topo.gui.bindings.enabled.params.size(); j++)
        {
          param_topo_mapping m = that_param;
          m.param_index = that_topo.gui.bindings.enabled.params[j];
          that_values.push_back(_gui->gui_state()->get_plain_at(m).step());
        }
        enabled &= that_topo.gui.bindings.enabled.param_selector(that_values);
      }
      if (that_topo.gui.bindings.visible.param_selector != nullptr)
      {
        std::vector<int> that_values;
        for (int j = 0; j < that_topo.gui.bindings.visible.params.size(); j++)
        {
          param_topo_mapping m = that_param;
          m.param_index = that_topo.gui.bindings.visible.params[j];
          that_values.push_back(_gui->gui_state()->get_plain_at(m).step());
        }
        enabled &= that_topo.gui.bindings.visible.param_selector(that_values);
      }
      setItemEnabled(i + 1, enabled);
    }
  }
  ComboBox::showPopup();
}

}