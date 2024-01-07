#include <plugin_base/gui/controls.hpp>

#include <limits>
#include <algorithm>

using namespace juce;

namespace plugin_base {

static void
fill_popup_menu(param_domain const& domain, PopupMenu& menu, gui_submenu const* data, Colour const& subheader_color)
{
  menu.clear();
  for (int i = 0; i < data->indices.size(); i++)
    menu.addItem(data->indices[i] + 1, domain.raw_to_text(false, data->indices[i]));
  for(int i = 0; i < data->children.size(); i++)
  {
    if (data->children[i]->is_subheader)
      menu.addColouredItem(-1, data->children[i]->name, subheader_color, false, false, nullptr);
    else
    {
      PopupMenu child;
      fill_popup_menu(domain, child, data->children[i].get(), subheader_color);
      menu.addSubMenu(data->children[i]->name, child);
    }
  }
}

static void
fill_host_menu(PopupMenu& menu, std::vector<std::shared_ptr<host_menu_item>> const& children)
{
  for(int i = 0; i < children.size(); i++)
  {
    auto const& child = *children[i].get();
    if(child.flags & host_menu_flags_separator)
      menu.addSeparator();
    else if(child.children.empty())
      menu.addItem(child.tag + 1, child.name, child.flags & host_menu_flags_enabled, child.flags & host_menu_flags_checked);
    else
    {
      PopupMenu submenu;
      fill_host_menu(submenu, child.children);
      menu.addSubMenu(child.name, submenu, child.flags & host_menu_flags_enabled);
    }
  }
}

void 
text_button::mouseUp(MouseEvent const& e)
{
  if(!e.mods.isRightButtonDown())
    TextButton::mouseUp(e);
}

void
toggle_button::mouseUp(MouseEvent const& e)
{
  if (!e.mods.isRightButtonDown())
    ToggleButton::mouseUp(e);
}

void
menu_button::clicked()
{
  PopupMenu menu;
  menu.setLookAndFeel(&getLookAndFeel());
  for (int i = 0; i < _items.size(); i++)
    menu.addItem(i + 1, _items[i], true, i == _selected_index);
  PopupMenu::Options options;
  options = options.withTargetComponent(this);
  menu.showMenuAsync(options, [this](int id) {
    int index = id - 1;
    if (index == _selected_index) return;
    _selected_index = index;
    if (selected_index_changed != nullptr)
      selected_index_changed(index);
    });
}

std::string 
param_name_label::label_ref_text(param_desc const* param, bool short_)
{
  auto const& ref_text = param->param->gui.label_reference_text;
  if (ref_text.size()) return ref_text;
  return short_ ? param->param->info.tag.short_name : param->info.name;
}

last_tweaked_label::
last_tweaked_label(plugin_state const* state):
_state(state)
{
  state->add_any_listener(this);
  any_state_changed(0, state->get_plain_at_index(0));
}

void 
last_tweaked_label::any_state_changed(int index, plain_value plain)
{
  if(_state->desc().params[index]->param->dsp.direction == param_direction::output) return;
  setText(_state->desc().params[index]->full_name, dontSendNotification);
}

last_tweaked_editor::
last_tweaked_editor(plugin_state* state, lnf* lnf) :
_state(state)
{
  addListener(this);
  setFont(lnf->font());
  state->add_any_listener(this);
  setJustification(Justification::centredRight);
  any_state_changed(0, state->get_plain_at_index(0));
}

last_tweaked_editor::
~last_tweaked_editor()
{
  _state->remove_any_listener(this);
  removeListener(this);
}

void
last_tweaked_editor::any_state_changed(int index, plain_value plain)
{
  if(_updating) return;
  if (_state->desc().params[index]->param->dsp.direction == param_direction::output) return;
  _last_tweaked = index;
  setText(String(_state->desc().params[index]->param->domain.plain_to_text(false, plain)), dontSendNotification);
}

void 
last_tweaked_editor::textEditorTextChanged(TextEditor& te)
{
  plain_value plain;
  if(_last_tweaked == -1) return;
  std::string value = te.getText().toStdString();
  if(!_state->desc().params[_last_tweaked]->param->domain.text_to_plain(false, value, plain)) return;
  _updating = true;
  _state->set_plain_at_index(_last_tweaked, plain);
  _updating = false;
}

preset_button::
preset_button(plugin_gui* gui) :
_gui(gui), _presets(gui->gui_state()->desc().presets())
{ 
  set_items(vector_map(_presets, [](auto const& p) { return p.name; }));
  extra_state_changed();
  setButtonText("Preset");
  _gui->extra_state()->add_listener(factory_preset_key, this);
  selected_index_changed = [this](int index) {
    index = std::clamp(index, 0, (int)get_items().size());
    _gui->extra_state()->set_text(factory_preset_key, get_items()[index]);
    _gui->load_patch(_presets[index].path, true);
  };
}

void 
preset_button::extra_state_changed()
{
  std::string selected_preset = _gui->extra_state()->get_text(factory_preset_key, "");
  auto iter = std::find(get_items().begin(), get_items().end(), selected_preset);
  if (iter != get_items().end())
    set_selected_index((int)(iter - get_items().begin()));
}

image_component::
image_component(
  format_config const* config, 
  std::string const& file_name, 
  RectanglePlacement placement)
{
  String path((get_resource_location(config) / resource_folder_ui / file_name).string());
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

  int const hpadding = 19;
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

  // note that we never deregister since we cannot dynamic_cast in the destructor
  // should not be a problem as we are our own listener
  dynamic_cast<Component&>(*this).addMouseListener(this, true);
}

void 
param_component::mouseUp(MouseEvent const& evt)
{
  if(!evt.mods.isRightButtonDown()) return;
  auto& self = dynamic_cast<Component&>(*this);
  if(!self.isEnabled()) return;
  if(_param->param->dsp.direction == param_direction::output) return;

  PopupMenu menu;
  PopupMenu::Options options;
  options = options.withTargetComponent(self);
  menu.setLookAndFeel(&self.getLookAndFeel());

  bool have_menu = false;
  std::unique_ptr<param_menu_handler> plugin_handler = {};
  param_menu_handler_factory plugin_handler_factory = _param->param->gui.menu_handler_factory;
  if(plugin_handler_factory)
    plugin_handler = plugin_handler_factory(_gui->gui_state());
  if (plugin_handler)
  {
    auto plugin_menus = plugin_handler->menus();
    for (int m = 0; m < plugin_menus.size(); m++)
    {
      have_menu = true;
      if (!plugin_menus[m].name.empty())
        menu.addColouredItem(-1, plugin_menus[m].name, _module->module->gui.colors.tab_text, false, false, nullptr);
      for (int e = 0; e < plugin_menus[m].entries.size(); e++)
        menu.addItem(10000 + m * 1000 + e * 100, plugin_menus[m].entries[e].title);
    }
  }

  auto host_menu = _gui->gui_state()->desc().config->context_menu(_param->info.id_hash);
  if (host_menu && host_menu->root.children.size())
  {
    have_menu = true;
    menu.addColouredItem(-1, "Host", _module->module->gui.colors.tab_text, false, false, nullptr);
    fill_host_menu(menu, host_menu->root.children);
  }

  if(!have_menu) return;

  menu.showMenuAsync(options, [this, host_menu = host_menu.release(), plugin_handler = plugin_handler.release()](int id) {
    if(1 <= id && id < 10000) host_menu->clicked(id - 1);
    else if(10000 <= id)
    {
      auto plugin_menus = plugin_handler->menus();
      auto const& menu = plugin_menus[(id - 10000) / 1000];
      auto const& action_entry = menu.entries[((id - 10000) % 1000) / 100];
      _gui->gui_state()->begin_undo_region();
      plugin_handler->execute(menu.menu_id, action_entry.action, _module->info.topo, _module->info.slot, _param->info.topo, _param->info.slot);      
      _gui->gui_state()->end_undo_region(action_entry.title);
    }
    delete host_menu;
    delete plugin_handler;
  });
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
  setTooltip(_param->param->info.tag.name + ": " + _param->param->domain.plain_to_text(false, plain));
}

void 
param_toggle_button::buttonStateChanged(Button*)
{ 
  if(_checked == getToggleState()) return;
  plain_value plain = _param->param->domain.raw_to_plain(getToggleState() ? 1 : 0);
  _checked = getToggleState();
  _gui->param_changed(_param->info.global, plain);
}

param_toggle_button::
param_toggle_button(plugin_gui* gui, module_desc const* module, param_desc const* param):
param_component(gui, module, param), autofit_togglebutton()
{
  auto value = param->param->domain.default_plain(module->info.slot, param->info.slot);
  setTooltip(_param->param->info.tag.name + ": " + _param->param->domain.plain_to_text(false, value));
  _checked = value.step() != 0;
  addListener(this);
  init();
}

param_slider::
param_slider(plugin_gui* gui, module_desc const* module, param_desc const* param) :
param_component(gui, module, param), Slider()
{
  auto tooltip = param->param->gui.tooltip;
  switch (tooltip)
  {
  case gui_label_contents::none: break;
  case gui_label_contents::name: setTooltip(param->param->info.tag.name); break;
  case gui_label_contents::value: setPopupDisplayEnabled(true, true, nullptr); break;
  case gui_label_contents::short_name: setTooltip(param->param->info.tag.short_name); break;
  default: assert(false); break;
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
  {
    auto const& color = module->module->gui.colors.tab_text;
    fill_popup_menu(domain, *getRootMenu(), param_gui.submenu.get(), color);
  }
  autofit();
  addListener(this);
  setEditableText(false);
  init();
}

void 
param_combobox::own_param_changed(plain_value plain)
{
  setSelectedId(plain.step() + 1 - _param->param->domain.min, dontSendNotification);
  std::string value = _param->param->domain.plain_to_text(false, plain);
  std::string name = _param->param->info.tag.name;
  setTooltip(name + ": " + value);
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