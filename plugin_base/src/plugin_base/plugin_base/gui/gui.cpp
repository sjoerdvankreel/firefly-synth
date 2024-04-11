#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/controls.hpp>
#include <plugin_base/gui/containers.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/io_user.hpp>
#include <plugin_base/shared/io_plugin.hpp>

#include <vector>
#include <algorithm>

using namespace juce;

namespace plugin_base {

static int const margin_param = 1;
static int const margin_module = 2;
static int const margin_content = 2;
static int const margin_vsection = 2;
static int const margin_hsection = 3;

static std::vector<std::string> tab_menu_module_actions = { 
  "", "Clear", "Clear All", "Delete", "Insert Before", "Insert After", "Copy To", "Move To", "Swap With" };

static void
fill_module_tab_menu(PopupMenu& menu, int base_id, int slot, int slots, std::set<int> const& actions)
{
  if(actions.contains(module_tab_menu_handler::clear))
    menu.addItem(base_id + module_tab_menu_handler::clear * 100,
      tab_menu_module_actions[module_tab_menu_handler::clear]);
  if (slots > 1)
  {
    if (actions.contains(module_tab_menu_handler::clear_all))
      menu.addItem(base_id + module_tab_menu_handler::clear_all * 100,
        tab_menu_module_actions[module_tab_menu_handler::clear_all]);
    if (actions.contains(module_tab_menu_handler::delete_))
      menu.addItem(base_id + module_tab_menu_handler::delete_ * 100,
        tab_menu_module_actions[module_tab_menu_handler::delete_]);
    if (actions.contains(module_tab_menu_handler::insert_before))
      menu.addItem(base_id + module_tab_menu_handler::insert_before * 100,
        tab_menu_module_actions[module_tab_menu_handler::insert_before]);
    if (actions.contains(module_tab_menu_handler::insert_after))
      menu.addItem(base_id + module_tab_menu_handler::insert_after * 100,
        tab_menu_module_actions[module_tab_menu_handler::insert_after], slot < slots - 1);

    if (actions.contains(module_tab_menu_handler::copy_to))
    {
      PopupMenu copy_menu;
        for (int i = 0; i < slots; i++)
          copy_menu.addItem(base_id + module_tab_menu_handler::copy_to * 100 + i, std::to_string(i + 1), i != slot);
      menu.addSubMenu(tab_menu_module_actions[module_tab_menu_handler::copy_to], copy_menu);
    }

    if (actions.contains(module_tab_menu_handler::move_to))
    {
      PopupMenu move_menu;
      for (int i = 0; i < slots; i++)
        move_menu.addItem(base_id + module_tab_menu_handler::move_to * 100 + i, std::to_string(i + 1), i != slot);
      menu.addSubMenu(tab_menu_module_actions[module_tab_menu_handler::move_to], move_menu);
    }

    if (actions.contains(module_tab_menu_handler::swap_with))
    {
      PopupMenu swap_menu;
      for (int i = 0; i < slots; i++)
        swap_menu.addItem(base_id + module_tab_menu_handler::swap_with * 100 + i, std::to_string(i + 1), i != slot);
      menu.addSubMenu(tab_menu_module_actions[module_tab_menu_handler::swap_with], swap_menu);
    }
  }
}

static Justification 
justification_type(gui_label const& label)
{
  switch (label.align)
  {
  case gui_label_align::top:
    switch (label.justify) {
    case gui_label_justify::center: return Justification::centred;
    case gui_label_justify::far: return Justification::centredTop;
    case gui_label_justify::near: return Justification::centredBottom;
    default: assert(false); break; }
    break;
  case gui_label_align::bottom:
    switch (label.justify) {
    case gui_label_justify::center: return Justification::centred;
    case gui_label_justify::near: return Justification::centredTop;
    case gui_label_justify::far: return Justification::centredBottom;
    default: assert(false); break; }
    break;
  case gui_label_align::left:
    switch (label.justify) {
    case gui_label_justify::center: return Justification::centred;
    case gui_label_justify::far: return Justification::centredLeft;
    case gui_label_justify::near: return Justification::centredRight;
    default: assert(false); break; }
    break;
  case gui_label_align::right:
    switch (label.justify) {
    case gui_label_justify::center: return Justification::centred;
    case gui_label_justify::far: return Justification::centredRight;
    case gui_label_justify::near: return Justification::centredLeft;
    default: assert(false); break; }
    break;
  default:
    assert(false);
    break;
  }
  assert(false);
  return Justification::centred;
}

static int
module_header_height(int font_height)
{ return font_height + 6; }

std::vector<int>
gui_vertical_distribution(int total_height, int font_height,
  std::vector<gui_vertical_section_size> const& section_sizes)
{
  float total_vsection_size = 0;
  int total_module_fixed_height = 0;
  int header_height = module_header_height(font_height);
  std::vector<float> result;
  std::vector<float> module_fixed_height(section_sizes.size(), 0.0f);
  for (int i = 0; i < section_sizes.size(); i++)
  {
    total_vsection_size += section_sizes[i].size_relative;
    module_fixed_height[i] += margin_module;
    module_fixed_height[i] += section_sizes[i].header ? header_height + margin_module : 0;
    total_module_fixed_height += module_fixed_height[i];
  }
  int total_remaining_height = total_height - total_module_fixed_height;
  for (int i = 0; i < section_sizes.size(); i++)
  {
    float remaining_portion = section_sizes[i].size_relative / (float)total_vsection_size * total_remaining_height;
    result.push_back(module_fixed_height[i] + remaining_portion);
  }
  return vector_map(result, [](auto const& val) { return (int)(val * 100); });
}

void
gui_param_listener::gui_param_changed(int index, plain_value plain)
{
  gui_param_begin_changes(index);
  gui_param_changing(index, plain);
  gui_param_end_changes(index);
}

void
gui_hover_listener::mouseExit(MouseEvent const&)
{
  switch (_type)
  {
  case gui_hover_type::param: _gui->param_mouse_exit(_global_index); break;
  case gui_hover_type::module: _gui->module_mouse_exit(_global_index); break;
  case gui_hover_type::custom: _gui->custom_mouse_exit(_global_index); break;
  default: assert(false); break;
  }
}

void
gui_hover_listener::mouseEnter(MouseEvent const&)
{
  switch (_type)
  {
  case gui_hover_type::param: _gui->param_mouse_enter(_global_index); break;
  case gui_hover_type::module: _gui->module_mouse_enter(_global_index); break;
  case gui_hover_type::custom: _gui->custom_mouse_enter(_global_index); break;
  default: assert(false); break;
  }
}

void 
gui_undo_listener::mouseUp(MouseEvent const& event)
{
  if(!event.mods.isRightButtonDown()) return;

  // no way to suppress event propagation so we need to filter a bit
  if(dynamic_cast<Label*>(event.originalComponent)) return; // needed for comboboxes
  if(dynamic_cast<Button*>(event.originalComponent)) return;
  if(dynamic_cast<TabBarButton*>(event.originalComponent)) return;
  if(dynamic_cast<param_component*>(event.originalComponent)) return;

  juce::PopupMenu menu;
  juce::PopupMenu::Options options;
  menu.setLookAndFeel(&_gui->getLookAndFeel());
  // single child is where content scaling is applied
  options = options.withTargetComponent(_gui->getChildComponent(0));
  options = options.withMousePosition();

  juce::PopupMenu undo_menu;
  auto undo_stack = _gui->gui_state()->undo_stack();
  for(int i = 0; i < undo_stack.size(); i++)
    undo_menu.addItem(i + 1, undo_stack[i]);
  menu.addSubMenu("Undo", undo_menu);

  juce::PopupMenu redo_menu;
  auto redo_stack = _gui->gui_state()->redo_stack();
  for (int i = 0; i < redo_stack.size(); i++)
    redo_menu.addItem(i + 1000 + 1, redo_stack[i]);
  menu.addSubMenu("Redo", redo_menu);

  menu.addItem(2001, "Copy Patch");
  menu.addItem(2002, "Paste Patch");

  menu.showMenuAsync(options, [this](int result) {
    if(1 <= result && result < 1000)
      _gui->gui_state()->undo(result - 1);
    else if(1001 <= result && result < 2000)
      _gui->gui_state()->redo(result - 1001);
    else if (2001 == result)
    {
      auto state = plugin_io_save_state(*_gui->gui_state());
      state.push_back('\0');
      juce::SystemClipboard::copyTextToClipboard(juce::String(state.data()));
    }
    else if (2002 == result)
    {
      plugin_state new_state(&_gui->gui_state()->desc(), false);
      auto clip_contents = juce::SystemClipboard::getTextFromClipboard().toStdString();
      std::vector<char> clip_data(clip_contents.begin(), clip_contents.end());
      auto load_result = plugin_io_load_state(clip_data, new_state);
      if (load_result.ok() && !load_result.warnings.size())
      {
        _gui->gui_state()->begin_undo_region();
        _gui->gui_state()->copy_from(new_state.state());
        _gui->gui_state()->end_undo_region("Paste", "Patch");
      }
      else
      {
        std::string message = "Clipboard does not contain valid patch data.";
        auto options = MessageBoxOptions::makeOptionsOk(MessageBoxIconType::WarningIcon, "Error", message, String());
        options = options.withAssociatedComponent(_gui->getChildComponent(0));
        AlertWindow::showAsync(options, nullptr);
        return;
      }
    }
  });
}

void 
gui_tab_menu_listener::mouseUp(MouseEvent const& event)
{
  if(!event.mods.isRightButtonDown()) return;
  auto const& topo = _state->desc().plugin->modules[_module];
  auto colors = _lnf->module_gui_colors(topo.info.tag.full_name);
  int slots = topo.info.slot_count;

  PopupMenu menu;
  std::unique_ptr<module_tab_menu_handler> handler = {};
  if(topo.gui.menu_handler_factory != nullptr)
  {
    handler = topo.gui.menu_handler_factory(_state);
    auto module_menus = handler->module_menus();
    for(int m = 0; m < module_menus.size(); m++)
    {
      PopupMenu dummy_menu;
      fill_module_tab_menu(dummy_menu, m * 1000, _slot, slots, module_menus[m].actions);
      if(dummy_menu.getNumItems() > 0)
      {
        if(!module_menus[m].name.empty())
          menu.addColouredItem(-1, module_menus[m].name, colors.tab_text, false, false, nullptr);
        fill_module_tab_menu(menu, m * 1000, _slot, slots, module_menus[m].actions);
      }
    }
    auto custom_menus = handler->custom_menus();
    for(int m = 0; m < custom_menus.size(); m++)
    {
      if (!custom_menus[m].name.empty())
        menu.addColouredItem(-1, custom_menus[m].name, colors.tab_text, false, false, nullptr);
      for(int e = 0; e < custom_menus[m].entries.size(); e++)
        menu.addItem(10000 + m * 1000 + e * 100, custom_menus[m].entries[e].title);
    }
  }

  PopupMenu::Options options;
  options = options.withTargetComponent(_button);
  menu.setLookAndFeel(&_button->getLookAndFeel());
  menu.showMenuAsync(options, [this, handler = handler.release()](int id) {
    module_tab_menu_result result("", false, "", "");
    auto custom_menus = handler->custom_menus();
    auto module_menus = handler->module_menus();
    if (0 < id && id < 10000)
    {
      int action_id = (id % 1000) / 100;
      int menu_id = module_menus[id / 1000].menu_id;
      _state->begin_undo_region();
      result = handler->execute_module(menu_id, action_id, _module, _slot, id % 100);
      _state->end_undo_region(tab_menu_module_actions[action_id], result.item());
    }
    else if(10000 <= id && id < 20000)
    {
      auto const& menu = custom_menus[(id - 10000) / 1000];
      auto const& action_entry = menu.entries[((id - 10000) % 1000) / 100];
      _state->begin_undo_region();
      result = handler->execute_custom(menu.menu_id, action_entry.action, _module, _slot);
      _state->end_undo_region(action_entry.title, result.item());
    }
    delete handler;
    if(!result.show_warning()) return;
    auto options = MessageBoxOptions::makeOptionsOk(MessageBoxIconType::WarningIcon, result.title(), result.content());
    options = options.withAssociatedComponent(_gui->getChildComponent(0));
    AlertWindow::showAsync(options, [](int){});
  });
}

std::string 
module_section_tab_key(plugin_topo const& topo, int section_index)
{ return topo.gui.module_sections[section_index].id + "/" + extra_state_tab_index; }

std::set<std::string>
gui_extra_state_keyset(plugin_topo const& topo)
{
  std::set<std::string> result = {};
  result.insert(factory_preset_key);
  for (int i = 0; i < topo.modules.size(); i++)
    if (topo.modules[i].info.slot_count > 1)
      result.insert(topo.modules[i].info.tag.id + "/" + extra_state_tab_index);
  for (int i = 0; i < topo.gui.module_sections.size(); i++)
    if (topo.gui.module_sections[i].tabbed)
      result.insert(module_section_tab_key(topo, i));
  return result;
}

plugin_gui::
~plugin_gui() 
{ 
  setLookAndFeel(nullptr);
  removeMouseListener(&_undo_listener);
}

plugin_gui::
plugin_gui(plugin_state* gui_state, plugin_base::extra_state* extra_state):
_gui_state(gui_state), _undo_listener(this), _extra_state(extra_state)
{
  setOpaque(true);
  addMouseListener(&_undo_listener, true);
  auto const& topo = *gui_state->desc().plugin;
  theme_changed(user_io_load_list(topo, user_io::base, user_state_theme_key, topo.gui.default_theme, gui_state->desc().themes()));
}

void
plugin_gui::theme_changed(std::string const& theme_name)
{
  // don't just clear out everything!
  // only stuff that gets rebuild by the gui must be reset
  // stuff that get bound by the plugin (e.g. param_listener) must remain
  
  removeAllChildren();
  _tooltip = {};
  setLookAndFeel(nullptr);

  // drop listeners before components
  // they deregister themselves from components
  _hover_listeners.clear();
  _tab_menu_listeners.clear();
  _gui_mouse_listeners.clear();
  _tab_selection_listeners.clear();

  // drop components before lnfs
  _components.clear();

  // finally drop lnfs
  _module_lnfs.clear();
  _custom_lnfs.clear();

  _lnf = std::make_unique<lnf>(&gui_state()->desc(), theme_name, -1, -1, -1);
  setLookAndFeel(_lnf.get());
  auto const& topo = *gui_state()->desc().plugin;
  bool is_fx = topo.type == plugin_type::fx;

  for (int i = 0; i < gui_state()->desc().plugin->gui.custom_sections.size(); i++)
    _custom_lnfs[i] = std::make_unique<lnf>(&_gui_state->desc(), _lnf->theme(), i, -1, -1);
  for (int i = 0; i < gui_state()->desc().plugin->modules.size(); i++)
    _module_lnfs[i] = std::make_unique<lnf>(&_gui_state->desc(), _lnf->theme(), -1, gui_state()->desc().plugin->modules[i].gui.section, i);

  // note: default width and aspect ratios are contained in theme
  add_and_make_visible(*this, make_content());
  int default_width = _lnf->theme_settings().get_default_width(is_fx);
  float ratio = _lnf->theme_settings().get_aspect_ratio_height(is_fx) / (float)_lnf->theme_settings().get_aspect_ratio_width(is_fx);
  getChildComponent(0)->setSize(default_width, default_width * ratio);
  float user_scale = user_io_load_num(topo, user_io::base, user_state_scale_key, 1.0,
    _lnf->theme_settings().min_scale, _lnf->theme_settings().max_scale);
  float w = default_width * user_scale * _system_dpi_scale;
  setSize(w, w * ratio);
  resized();
  _tooltip = std::make_unique<TooltipWindow>(getChildComponent(0));
} 

void
plugin_gui::param_changed(int index, plain_value plain)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _param_listeners.size(); i++)
      _param_listeners[i]->gui_param_changed(index, plain);
}

void
plugin_gui::param_begin_changes(int index)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _param_listeners.size(); i++)
      _param_listeners[i]->gui_param_begin_changes(index);
}

void
plugin_gui::param_end_changes(int index)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _param_listeners.size(); i++)
      _param_listeners[i]->gui_param_end_changes(index);
}

void
plugin_gui::param_changing(int index, plain_value plain)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _param_listeners.size(); i++)
      _param_listeners[i]->gui_param_changing(index, plain);
}

void
plugin_gui::remove_gui_mouse_listener(gui_mouse_listener* listener)
{
  auto iter = std::find(_gui_mouse_listeners.begin(), _gui_mouse_listeners.end(), listener);
  if (iter != _gui_mouse_listeners.end()) _gui_mouse_listeners.erase(iter);
}

void
plugin_gui::remove_param_listener(gui_param_listener* listener)
{
  auto iter = std::find(_param_listeners.begin(), _param_listeners.end(), listener);
  if (iter != _param_listeners.end()) _param_listeners.erase(iter);
}

void
plugin_gui::remove_tab_selection_listener(gui_tab_selection_listener* listener)
{
  auto iter = std::find(_tab_selection_listeners.begin(), _tab_selection_listeners.end(), listener);
  if (iter != _tab_selection_listeners.end()) _tab_selection_listeners.erase(iter);
}

void
plugin_gui::fire_state_loaded()
{
  for(int i = 0; i < _gui_state->desc().param_count; i++)
    param_changed(i, _gui_state->get_plain_at_index(i));
}

void
plugin_gui::param_mouse_exit(int param)
{
  if (_last_mouse_enter_param == param) return;
  for (int i = 0; i < _gui_mouse_listeners.size(); i++)
    _gui_mouse_listeners[i]->param_mouse_exit(param);
  _last_mouse_enter_param = -1;
}

void
plugin_gui::param_mouse_enter(int param)
{
  if(_last_mouse_enter_param == param) return;
  for (int i = 0; i < _gui_mouse_listeners.size(); i++)
    _gui_mouse_listeners[i]->param_mouse_enter(param);
  _last_mouse_enter_param = param;
} 

void
plugin_gui::custom_mouse_exit(int section)
{
  if (_last_mouse_enter_custom == section) return;
  for (int i = 0; i < _gui_mouse_listeners.size(); i++)
    _gui_mouse_listeners[i]->custom_mouse_exit(section);
  _last_mouse_enter_custom = -1;
}

void
plugin_gui::custom_mouse_enter(int section)
{
  if(_last_mouse_enter_custom == section) return;
  // tooltip may not be there yet during theme switching
  if(_tooltip) _tooltip->setLookAndFeel(_custom_lnfs[section].get());
  for (int i = 0; i < _gui_mouse_listeners.size(); i++)
    _gui_mouse_listeners[i]->custom_mouse_enter(section);
  _last_mouse_enter_custom = section;
}

void
plugin_gui::module_mouse_exit(int module)
{
  if (_last_mouse_enter_module == module) return;
  for (int i = 0; i < _gui_mouse_listeners.size(); i++)
    _gui_mouse_listeners[i]->module_mouse_exit(module);
  _last_mouse_enter_module = -1;
}

void
plugin_gui::module_mouse_enter(int module)
{
  if(_last_mouse_enter_module == module) return;
  int index = gui_state()->desc().modules[module].module->info.index;
  // tooltip may not be there yet during theme switching
  if(_tooltip) _tooltip->setLookAndFeel(_module_lnfs[index].get());
  for(int i = 0; i < _gui_mouse_listeners.size(); i++)
    _gui_mouse_listeners[i]->module_mouse_enter(module);
  _last_mouse_enter_module = module;
}

void
plugin_gui::add_tab_menu_listener(juce::TabBarButton& button, int module, int slot)
{
  auto listener = std::make_unique<gui_tab_menu_listener>(this, gui_state(), _lnf.get(), &button, module, slot);
  _tab_menu_listeners.push_back(std::move(listener));
}

void
plugin_gui::add_hover_listener(Component& component, gui_hover_type type, int global_index)
{
  auto listener = std::make_unique<gui_hover_listener>(this, &component, type, global_index);
  _hover_listeners.push_back(std::move(listener));
}

template <class T, class... U> T&
plugin_gui::make_component(U&&... args) 
{
  auto component = std::make_unique<T>(std::forward<U>(args)...);
  T* result = component.get();
  _components.emplace_back(std::move(component));
  return *result;
}

void 
plugin_gui::set_system_dpi_scale(float scale)
{
  float factor = scale / _system_dpi_scale;
  _system_dpi_scale = scale;
  setSize(getLocalBounds().getWidth() * factor, getLocalBounds().getHeight() * factor);
}

void
plugin_gui::resized()
{
  float w = getLocalBounds().getWidth();
  bool is_fx = _gui_state->desc().plugin->type == plugin_type::fx;
  float user_scale = (w / _lnf->theme_settings().get_default_width(is_fx)) / _system_dpi_scale;
  getChildComponent(0)->setTransform(AffineTransform::scale(user_scale * _system_dpi_scale));
  user_io_save_num(*_gui_state->desc().plugin, user_io::base, user_state_scale_key, user_scale);
}

graph_engine* 
plugin_gui::get_module_graph_engine(module_topo const& module)
{
  if(module.graph_engine_factory == nullptr) return nullptr;
  auto iter = _module_graph_engines.find(module.info.index);
  if(iter != _module_graph_engines.end()) return iter->second.get();
  _module_graph_engines[module.info.index] = module.graph_engine_factory(&_gui_state->desc());
  return _module_graph_engines[module.info.index].get();
}

void
plugin_gui::init_multi_tab_component(tab_component& tab, std::string const& id, int module_index, int section_index)
{
  assert((module_index == -1) != (section_index == -1));
  tab.tab_changed = [this, id, module_index, section_index](int tab_index) { 
    set_extra_state_num(id, extra_state_tab_index, tab_index);
    if(module_index != -1)
      for(int i = 0; i < _tab_selection_listeners.size(); i++)
        _tab_selection_listeners[i]->module_tab_changed(module_index, tab_index);
    if (section_index != -1)
      for (int i = 0; i < _tab_selection_listeners.size(); i++)
        _tab_selection_listeners[i]->section_tab_changed(section_index, tab_index);
  };
  tab.setCurrentTabIndex(std::clamp((int)get_extra_state_num(id, extra_state_tab_index, 0), 0, tab.getNumTabs() - 1));
  set_extra_state_num(id, extra_state_tab_index, tab.getCurrentTabIndex());
}

void
plugin_gui::reloaded()
{
  auto const& topo = *_gui_state->desc().plugin;
  auto settings = _lnf->theme_settings();
  bool is_fx = _gui_state->desc().plugin->type == plugin_type::fx;
  int default_width = settings.get_default_width(is_fx);
  float ratio = settings.get_aspect_ratio_height(is_fx) / (float)settings.get_aspect_ratio_width(is_fx);
  float user_scale = user_io_load_num(topo, user_io::base, user_state_scale_key, 1.0,
    settings.min_scale, settings.max_scale);
  float w = default_width * user_scale * _system_dpi_scale;
  setSize(w, (int)(w * ratio));
}

Component&
plugin_gui::make_content()
{
  auto const& topo = *_gui_state->desc().plugin;
  auto& grid = make_component<grid_component>(topo.gui.dimension_factory(_lnf->theme_settings()), margin_module, margin_module, 0, 0);
  for(int s = 0; s < topo.gui.custom_sections.size(); s++)
    grid.add(make_custom_section(topo.gui.custom_sections[s]), topo.gui.custom_sections[s].position);
  for(int s = 0; s < topo.gui.module_sections.size(); s++)
    if(topo.gui.module_sections[s].visible)
      grid.add(make_module_section(topo.gui.module_sections[s]), topo.gui.module_sections[s].position);
  return make_component<margin_component>(&grid, BorderSize<int>(margin_content));
}

Component& 
plugin_gui::make_custom_section(custom_section_gui const& section)
{
  int radius = _lnf->theme_settings().module_corner_radius;
  int vpadding = _lnf->theme_settings().param_section_vpadding;
  auto colors = _lnf->section_gui_colors(section.full_name);
  auto outline1 = colors.section_outline1;
  auto outline2 = colors.section_outline2;
  auto background1 = colors.section_background1;
  auto background2 = colors.section_background2;
  auto store = [this](std::unique_ptr<Component>&& owned) -> Component& { 
    auto result = owned.get(); 
    _components.emplace_back(std::move(owned)); 
    return *result; 
  };      
  lnf* lnf = custom_lnf(section.index);
  auto& content = section.gui_factory(this, lnf, store);
  auto& content_outline = make_component<rounded_container>(&content, radius, vpadding, 0, false, 
    rounded_container_mode::both, background1, background2, outline1, outline2);
  auto& result = make_component<rounded_container>(&content_outline, radius, 0, 0, true, 
    rounded_container_mode::fill, background1, background2, outline1, outline2);
  result.setLookAndFeel(lnf);
  add_hover_listener(result, gui_hover_type::custom, section.index);
  return result;
}

tab_component&
plugin_gui::make_tab_component(std::string const& id, std::string const& title, int module)
{
  auto& result = make_component<tab_component>(_extra_state, id + "/tab", TabbedButtonBar::Orientation::TabsAtTop);
  result.setOutline(0);
  result.setLookAndFeel(module_lnf(module));
  result.getTabbedButtonBar().setTitle(title);
  result.setTabBarDepth(module_header_height(_lnf->theme_settings().get_font_height()));
  return result;
}

void 
plugin_gui::add_component_tab(TabbedComponent& tc, Component& child, int module, std::string const& title)
{
  auto const& topo = *_gui_state->desc().plugin;
  int radius = _lnf->theme_settings().module_corner_radius;
  int module_slot = _gui_state->desc().modules[module].info.slot;
  int module_index = _gui_state->desc().modules[module].info.topo;
  auto colors = _lnf->module_gui_colors(topo.modules[module_index].info.tag.full_name);
  auto background1 = colors.tab_background1;
  auto background2 = colors.tab_background2;
  auto& corners = make_component<rounded_container>(&child, radius, 0, 0, true, rounded_container_mode::fill, background1, background2, background1, background2);
  tc.addTab(title, Colours::transparentBlack, &corners, false);
  auto tab_button = tc.getTabbedButtonBar().getTabButton(tc.getTabbedButtonBar().getNumTabs() - 1);
  add_hover_listener(*tab_button, gui_hover_type::module, module);
  if(topo.modules[module_index].gui.enable_tab_menu)
    add_tab_menu_listener(*tab_button, module_index, module_slot);
} 

Component&
plugin_gui::make_modules(module_desc const* slots)
{
  int index = slots[0].module->info.index;
  auto const& tag = slots[0].module->info.tag;
  auto& result = make_tab_component(tag.id, tag.display_name, index);
  for (int i = 0; i < slots[0].module->info.slot_count; i++)
    add_component_tab(result, make_param_sections(slots[i]), slots[i].info.global, std::to_string(i + 1));
  if(slots[0].module->info.slot_count > 1)
    init_multi_tab_component(result, tag.id, index, -1);
  return result;
}

Component&
plugin_gui::make_param_sections(module_desc const& module)
{
  int last_horizontal = 0;
  auto const& topo = *module.module;
  auto& result = make_component<grid_component>(topo.gui.dimension, margin_vsection, 0, topo.gui.autofit_row, topo.gui.autofit_column);
  for (int s = 0; s < topo.sections.size(); s++)
    last_horizontal = std::max(last_horizontal, topo.sections[s].gui.position.column);
  for (int s = 0; s < topo.sections.size(); s++)
    result.add(make_param_section(module, topo.sections[s], topo.sections[s].gui.position.column == last_horizontal), topo.sections[s].gui.position);
  add_hover_listener(result, gui_hover_type::module, module.info.global);
  return result;
}

Component&
plugin_gui::make_params(module_desc const& module, param_desc const* params)
{
  if (params[0].param->info.slot_count == 1)
    return make_single_param(module, params[0]);
  else
    return make_multi_param(module, params);
}

Component&
plugin_gui::make_single_param(module_desc const& module, param_desc const& param)
{
  Component* result = nullptr;
  if (param.param->gui.label.contents == gui_label_contents::none)
    result = &make_param_editor(module, param);
  else
    result = &make_param_label_edit(module, param);
  add_hover_listener(*result, gui_hover_type::param, param.info.global);
  return *result;
}

Component&
plugin_gui::make_multi_param(module_desc const& module, param_desc const* slots)
{
  auto const& param = slots[0].param;
  assert(param->gui.layout != param_layout::single_grid);
  bool vertical = param->gui.layout == param_layout::vertical;
  int autofit_row = param->gui.tabular && vertical ? 1 : 0;
  int autofit_column = param->gui.tabular && !vertical ? 1 : 0;
  auto& result = make_component<grid_component>(vertical, param->info.slot_count + (param->gui.tabular? 1: 0), 0, 0, autofit_row, autofit_column);
  if (param->gui.tabular)
  {
    std::string display_name = param->info.tag.display_name;
    auto colors = _lnf->module_gui_colors(module.module->info.tag.full_name);
    auto& header = make_component<autofit_label>(module_lnf(module.module->info.index), display_name, false, -1, true);
    header.setText(display_name, dontSendNotification);
    header.setColour(Label::ColourIds::textColourId, colors.table_header);
    result.add(header, vertical, 0);
  }
  for (int i = 0; i < param->info.slot_count; i++)
    result.add(make_single_param(module, slots[i]), vertical, i + (param->gui.tabular? 1: 0));
  return result;
}

Component&
plugin_gui::make_param_section(module_desc const& module, param_section const& section, bool last_horizontal)
{
  auto const& params = module.params;
  bool is_single_grid_param = false; 
  grid_component& grid = make_component<grid_component>(section.gui.dimension, margin_param, margin_param, 0, 0);
  
  for(int p = 0; p < module.module->params.size(); p++)
    if(module.module->params[p].gui.section == section.index)
      if (module.module->params[p].gui.layout == param_layout::single_grid)
      {
        assert(!is_single_grid_param);
        is_single_grid_param = true;
      }
  
  // multi-slot param being only param in section
  if (is_single_grid_param)
  {
    int slot = 0;
    for(int r = 0; r < section.gui.dimension.row_sizes.size(); r++)
      for (int c = 0; c < section.gui.dimension.column_sizes.size(); c += 2)
      {
        grid.add(make_param_label(module, params[slot], params[slot].param->gui.label.contents), { r, c });
        grid.add(make_param_editor(module, params[slot++]), { r, c + 1 });
      }
    assert(section.gui.scroll_mode == gui_scroll_mode::none);
    return make_component<param_section_container>(this, _lnf.get(), &module, &section, &grid, last_horizontal ? 0 : margin_hsection);
  }

  for (auto iter = params.begin(); iter != params.end(); iter += iter->param->info.slot_count)
    if(iter->param->gui.edit_type != gui_edit_type::none && iter->param->gui.section == section.index)
      if(section.gui.cell_split == gui_label_edit_cell_split::no_split)
        grid.add(make_params(module, &(*iter)), iter->param->gui.position);
      else
      {
        assert(iter->param->info.slot_count == 1);
        if (iter->param->gui.label.contents == gui_label_contents::none)
          grid.add(make_param_editor(module, *iter), iter->param->gui.position);
        else if (section.gui.cell_split == gui_label_edit_cell_split::horizontal)
        {
          assert(iter->param->gui.label.align == gui_label_align::left || iter->param->gui.label.align == gui_label_align::right);
          int label_dx = iter->param->gui.label.align == gui_label_align::left ? 0 : 1;
          int edit_dx = iter->param->gui.label.align == gui_label_align::left ? 1 : 0;
          grid.add(make_param_label(module, *iter, iter->param->gui.label.contents), iter->param->gui.position.move(0, label_dx));
          grid.add(make_param_editor(module, *iter), iter->param->gui.position.move(0, edit_dx));
        } else if (section.gui.cell_split == gui_label_edit_cell_split::vertical)
        {
          assert(iter->param->gui.label.align == gui_label_align::top || iter->param->gui.label.align == gui_label_align::bottom);
          int label_dy = iter->param->gui.label.align == gui_label_align::top ? 0 : 1;
          int edit_dy = iter->param->gui.label.align == gui_label_align::top ? 1 : 0;
          grid.add(make_param_label(module, *iter, iter->param->gui.label.contents), iter->param->gui.position.move(label_dy, 0));
          grid.add(make_param_editor(module, *iter), iter->param->gui.position.move(edit_dy, 0));
        } else assert(false);
      }

  if(section.gui.scroll_mode == gui_scroll_mode::none)
    return make_component<param_section_container>(this, _lnf.get(), &module, &section, &grid, last_horizontal ? 0 : margin_hsection);

  auto& viewer = make_component<autofit_viewport>(module_lnf(module.module->info.index));
  viewer.setViewedComponent(&grid, false);
  viewer.setScrollBarsShown(true, false);
  return make_component<param_section_container>(this, _lnf.get(), &module, &section, &viewer, 0);
}

Component&
plugin_gui::make_module_section(module_section_gui const& section)
{
  auto const& modules = _gui_state->desc().modules;
  if (!section.tabbed)
  {
    auto& grid = make_component<grid_component>(section.dimension, margin_module, margin_module, 0, 0);
    for (auto iter = modules.begin(); iter != modules.end(); iter += iter->module->info.slot_count)
      if (iter->module->gui.visible && iter->module->gui.section == section.index)
        grid.add(make_modules(&(*iter)), iter->module->gui.position);
    return grid;
  }

  int matched_module = -1;
  auto const& topo = *_gui_state->desc().plugin;
  for (int i = 0; i < topo.modules.size(); i++)
    if (topo.modules[i].gui.section == section.index)
      matched_module = i;
  assert(matched_module >= 0);
  auto& tabs = make_tab_component(section.id, "", matched_module);
  for(int o = 0; o < section.tab_order.size(); o++)
    for (auto iter = modules.begin(); iter != modules.end(); iter += iter->module->info.slot_count)
      if (iter->module->gui.visible && iter->module->gui.section == section.index && section.tab_order[o] == iter->module->info.index)
        add_component_tab(tabs, make_param_sections(*iter), iter->info.global, iter->module->gui.tabbed_name);
  init_multi_tab_component(tabs, section.id, -1, section.index);
  return tabs;
}

Label&
plugin_gui::make_param_label(module_desc const& module, param_desc const& param, gui_label_contents contents)
{
  Label* result = {};
  switch (contents)
  {
  case gui_label_contents::name:
    result = &make_component<param_name_label>(this, &module, &param, _module_lnfs[module.module->info.index].get());
    break;
  case gui_label_contents::value:
    result = &make_component<param_value_label>(this, &module, &param, _module_lnfs[module.module->info.index].get()); 
    break;
  default:
    assert(false);
    break;
  }
  result->setJustificationType(justification_type(param.param->gui.label));
  return *result;
}

Component&
plugin_gui::make_param_editor(module_desc const& module, param_desc const& param)
{
  auto colors = _lnf->module_gui_colors(module.module->info.tag.full_name);
  if(param.param->gui.edit_type == gui_edit_type::output)
  {
    auto& result = make_param_label(module, param, gui_label_contents::value);
    result.setColour(Label::ColourIds::textColourId, colors.control_text);
    return result;
  }

  if (param.param->gui.edit_type == gui_edit_type::output_module_name)
  {
    auto& result = make_component<module_name_label>(this, &module, &param, _module_lnfs[module.module->info.index].get());
    result.setColour(Label::ColourIds::textColourId, colors.control_text);
    return result;
  }

  Component* result = nullptr;
  switch (param.param->gui.edit_type)
  {
  case gui_edit_type::knob:
  case gui_edit_type::hslider:
  case gui_edit_type::vslider:
    result = &make_component<param_slider>(this, &module, &param); break;
  case gui_edit_type::toggle:
    result = &make_component<param_toggle_button>(this, &module, &param, _module_lnfs[module.module->info.index].get()); break;
  case gui_edit_type::list:
  case gui_edit_type::autofit_list:
    result = &make_component<param_combobox>(this, &module, &param, _module_lnfs[module.module->info.index].get()); break;
  default:
    assert(false);
    return *((Component*)nullptr);
  }

  return *result;
}

Component&
plugin_gui::make_param_label_edit(module_desc const& module, param_desc const& param)
{
  gui_dimension dimension;
  gui_position edit_position;
  gui_position label_position;

  switch (param.param->gui.label.align)
  {
  case gui_label_align::top:
    dimension = gui_dimension({ gui_dimension::auto_size, 1}, {1});
    label_position = { 0, 0 };
    edit_position = { 1, 0 };
    break;
  case gui_label_align::bottom:
    dimension = gui_dimension({ 1, gui_dimension::auto_size, }, { 1 });
    label_position = { 1, 0 };
    edit_position = { 0, 0 };
    break;
  case gui_label_align::left:
    dimension = gui_dimension({ 1 }, { gui_dimension::auto_size, 1});
    label_position = { 0, 0 };
    edit_position = { 0, 1 };
    break;
  case gui_label_align::right:
    dimension = gui_dimension({ 1 }, { 1, gui_dimension::auto_size });
    label_position = { 0, 1 };
    edit_position = { 0, 0 };
    break;
  default:
    assert(false);
    return *((Component*)nullptr);
  }

  auto& result = make_component<grid_component>(dimension, 0, 0, 0, 0);
  result.add(make_param_label(module, param, param.param->gui.label.contents), label_position);
  result.add(make_param_editor(module, param), edit_position);
  return result;
}

Component&
plugin_gui::make_init_button()
{
  auto& result = make_component<text_button>();
  result.setButtonText("Init");
  result.onClick = [this] { init_patch(); };
  return result;
}

Component&
plugin_gui::make_clear_button()
{
  auto& result = make_component<text_button>();
  result.setButtonText("Clear");
  result.onClick = [this] { clear_patch(); };
  return result;
}

Component&
plugin_gui::make_load_button()
{
  auto& result = make_component<text_button>();
  result.setButtonText("Load");
  result.onClick = [this] { load_patch(); };
  return result;
}

Component&
plugin_gui::make_save_button()
{
  auto& result = make_component<text_button>();
  result.setButtonText("Save");
  result.onClick = [this] { save_patch(); };
  return result;
}

void 
plugin_gui::init_patch()
{
  auto options = MessageBoxOptions::makeOptionsOkCancel(
    MessageBoxIconType::QuestionIcon, "Init Patch", "Are you sure?");
  options = options.withAssociatedComponent(getChildComponent(0));
  AlertWindow::showAsync(options, [this](int result) {
    if(result == 1)
    {
      _extra_state->clear();
      _gui_state->begin_undo_region();
      _gui_state->init(state_init_type::default_);
      fire_state_loaded();
      _gui_state->end_undo_region("Init", "Patch");
    }
  });
}

void
plugin_gui::clear_patch()
{
  auto options = MessageBoxOptions::makeOptionsOkCancel(
    MessageBoxIconType::QuestionIcon, "Clear Patch", "Are you sure?");
  options = options.withAssociatedComponent(getChildComponent(0));
  AlertWindow::showAsync(options, [this](int result) {
    if (result == 1)
    {
      _extra_state->clear();
      _gui_state->begin_undo_region();
      _gui_state->init(state_init_type::minimal);
      fire_state_loaded();
      _gui_state->end_undo_region("Clear", "Patch");
    }
  });
}

void
plugin_gui::save_patch()
{
  int save_flags = FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
  FileChooser* chooser = new FileChooser("Save Patch", File(), String("*.") + _gui_state->desc().plugin->extension, true, false, this);
  chooser->launchAsync(save_flags, [this](FileChooser const& chooser) {
    auto path = chooser.getResult().getFullPathName();
    delete& chooser;
    if (path.length() == 0) return;
    plugin_io_save_file_all(path.toStdString(), *_gui_state, *_extra_state);
    });
}

void
plugin_gui::load_patch()
{
  int load_flags = FileBrowserComponent::openMode;
  FileChooser* chooser = new FileChooser("Load Patch", File(), String("*.") + _gui_state->desc().plugin->extension, true, false, this);
  chooser->launchAsync(load_flags, [this](FileChooser const& chooser) {
    auto path = chooser.getResult().getFullPathName();
    delete& chooser;
    if (path.length() != 0)
      load_patch(path.toStdString(), false);
  });
}

void
plugin_gui::load_patch(std::string const& path, bool preset)
{
  _gui_state->begin_undo_region();
  auto icon = MessageBoxIconType::WarningIcon;
  auto result = plugin_io_load_file_all(path, *_gui_state, *_extra_state);
  if (result.error.size())
  {
    auto options = MessageBoxOptions::makeOptionsOk(icon, "Error", result.error, String(), this);
    options = options.withAssociatedComponent(getChildComponent(0));
    AlertWindow::showAsync(options, nullptr);
    return;
  }

  if(preset) _extra_state->clear();
  fire_state_loaded();
  _gui_state->end_undo_region("Load", preset ? "Preset" : "Patch");

  if (result.warnings.size())
  {
    String warnings;
    for (int i = 0; i < result.warnings.size() && i < 5; i++)
      warnings += String(result.warnings[i]) + "\n";
    if (result.warnings.size() > 5)
      warnings += String(std::to_string(result.warnings.size() - 5)) + " more...\n";
    auto options = MessageBoxOptions::makeOptionsOk(icon, "Warning", warnings, String(), this);
    options = options.withAssociatedComponent(getChildComponent(0));
    AlertWindow::showAsync(options, nullptr);
  }
}

}