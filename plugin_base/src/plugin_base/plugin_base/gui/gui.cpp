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
static int const margin_section = 2;
static int const margin_content = 2;

static std::string const extra_state_tab_index = "tab";
static std::string const user_state_width_key = "width";
static BorderSize<int> const param_section_border(16, 6, 6, 6);

static Justification 
justification_type(gui_label const& label)
{
  switch (label.align)
  {
  case gui_label_align::top:
  case gui_label_align::bottom:
    switch (label.justify) {
    case gui_label_justify::center: return Justification::centred;
    case gui_label_justify::near: return Justification::centredLeft;
    case gui_label_justify::far: return Justification::centredRight;
    default: break; }
    break;
  case gui_label_align::left:
    switch (label.justify) {
    case gui_label_justify::near: return Justification::topLeft;
    case gui_label_justify::far: return Justification::bottomLeft;
    case gui_label_justify::center: return Justification::centredLeft;
    default: break; }
    break;
  case gui_label_align::right:
    switch (label.justify) {
    case gui_label_justify::near: return Justification::topRight;
    case gui_label_justify::far: return Justification::bottomRight;
    case gui_label_justify::center: return Justification::centredRight;
    default: break; }
    break;
  default:
    break;
  }
  assert(false);
  return Justification::centred;
}

void
module_hover_listener::mouseExit(MouseEvent const&)
{
  _gui->module_mouse_exit(_module);
}

void
module_hover_listener::mouseEnter(MouseEvent const&)
{
  _gui->module_mouse_enter(_module);
}

void
gui_param_listener::gui_param_changed(int index, plain_value plain)
{
  gui_param_begin_changes(index);
  gui_param_changing(index, plain);
  gui_param_end_changes(index);
}

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
      result.insert(topo.gui.module_sections[i].id + "/" + extra_state_tab_index);
  return result;
}

plugin_gui::
plugin_gui(plugin_state* gui_state, plugin_base::extra_state* extra_state):
_lnf(&gui_state->desc(), -1, -1, -1), 
_gui_state(gui_state), _extra_state(extra_state)
{
  setOpaque(true);
  setLookAndFeel(&_lnf);
  auto const& topo = *gui_state->desc().plugin;
  
  for(int i = 0; i < gui_state->desc().plugin->gui.custom_sections.size(); i++)
    _custom_lnfs[i] = std::make_unique<lnf>(&_gui_state->desc(), i, -1, -1);
  for(int i = 0; i < gui_state->desc().plugin->modules.size(); i++)
    _module_lnfs[i] = std::make_unique<lnf>(& _gui_state->desc(), -1, gui_state->desc().plugin->modules[i].gui.section, i);

  add_and_make_visible(*this, make_content());
  float ratio = topo.gui.aspect_ratio_height / (float)topo.gui.aspect_ratio_width;
  getChildComponent(0)->setSize(topo.gui.min_width, topo.gui.min_width * ratio);
  float w = user_io_load_num(topo, user_io::base, user_state_width_key, topo.gui.min_width, topo.gui.min_width, std::numeric_limits<int>::max());
  setSize(w, topo.gui.min_width * ratio);
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
plugin_gui::remove_gui_listener(gui_listener* listener)
{
  auto iter = std::find(_gui_listeners.begin(), _gui_listeners.end(), listener);
  if (iter != _gui_listeners.end()) _gui_listeners.erase(iter);
}

void
plugin_gui::remove_param_listener(gui_param_listener* listener)
{
  auto iter = std::find(_param_listeners.begin(), _param_listeners.end(), listener);
  if (iter != _param_listeners.end()) _param_listeners.erase(iter);
}

void
plugin_gui::fire_state_loaded()
{
  for(int i = 0; i < _gui_state->desc().param_count; i++)
    param_changed(i, _gui_state->get_plain_at_index(i));
}

void
plugin_gui::module_mouse_exit(int module)
{
  for (int i = 0; i < _gui_listeners.size(); i++)
    _gui_listeners[i]->module_mouse_exit(module);
}

void
plugin_gui::module_mouse_enter(int module)
{
  for(int i = 0; i < _gui_listeners.size(); i++)
    _gui_listeners[i]->module_mouse_enter(module);
}

void
plugin_gui::add_hover_listener(Component& component, int module)
{
  auto listener = std::make_unique<module_hover_listener>(this, &component, module);
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
plugin_gui::resized()
{
  float w = getLocalBounds().getWidth();
  float scale = w / _gui_state->desc().plugin->gui.min_width;
  getChildComponent(0)->setTransform(AffineTransform::scale(scale));
  user_io_save_num(*_gui_state->desc().plugin, user_io::base, user_state_width_key, w);
}

void
plugin_gui::init_multi_tab_component(tab_component& tab, std::string const& id)
{
  tab.tab_changed = [this, id](int index) { set_extra_state_num(id, extra_state_tab_index, index); };
  tab.setCurrentTabIndex(std::clamp((int)get_extra_state_num(id, extra_state_tab_index, 0), 0, tab.getNumTabs() - 1));
  set_extra_state_num(id, extra_state_tab_index, tab.getCurrentTabIndex());
}

void
plugin_gui::reloaded()
{
  auto const& topo = *_gui_state->desc().plugin;
  float ratio = topo.gui.aspect_ratio_height / (float)topo.gui.aspect_ratio_width;
  float w = user_io_load_num(topo, user_io::base, user_state_width_key, topo.gui.min_width, topo.gui.min_width, std::numeric_limits<int>::max());
  setSize(w, topo.gui.min_width * ratio);
}

Component&
plugin_gui::make_content()
{
  auto const& topo = *_gui_state->desc().plugin;
  auto& grid = make_component<grid_component>(topo.gui.dimension, margin_module);
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
  auto const& topo = *_gui_state->desc().plugin;
  int radius = topo.gui.section_corner_radius;
  auto outline1 = section.colors.section_outline1;
  auto outline2 = section.colors.section_outline2;
  auto background1 = section.colors.custom_background1;
  auto background2 = section.colors.custom_background2;
  auto store = [this](std::unique_ptr<Component>&& owned) -> Component& { 
    auto result = owned.get(); 
    _components.emplace_back(std::move(owned)); 
    return *result; 
  };
  lnf* lnf = custom_lnf(section.index);
  auto& content = section.gui_factory(this, lnf, store);
  auto& content_outline = make_component<rounded_container>(&content, radius, false, false, outline1, outline2);
  auto& result = make_component<rounded_container>(&content_outline, radius, true, true, background1, background2);
  result.setLookAndFeel(lnf);
  return result;
}

tab_component&
plugin_gui::make_tab_component(std::string const& id, std::string const& title, int module)
{
  auto const& topo = *_gui_state->desc().plugin;
  auto& result = make_component<tab_component>(_extra_state, id + "/tab", TabbedButtonBar::Orientation::TabsAtTop);
  result.setOutline(0);
  result.getTabbedButtonBar().setTitle(title);
  result.setTabBarDepth(topo.gui.font_height + 4);
  result.setLookAndFeel(module_lnf(module));
  return result;
}

void 
plugin_gui::add_component_tab(TabbedComponent& tc, Component& child, int module, std::string const& title)
{
  auto const& topo = *_gui_state->desc().plugin;
  int radius = topo.gui.module_corner_radius;
  int module_index = _gui_state->desc().modules[module].info.topo;
  auto background1 = topo.modules[module_index].gui.colors.tab_background1;
  auto background2 = topo.modules[module_index].gui.colors.tab_background2;
  auto& corners = make_component<rounded_container>(&child, radius, true, true, background1, background2);
  tc.addTab(title, Colours::transparentBlack, &corners, false);
  auto tab_button = tc.getTabbedButtonBar().getTabButton(tc.getTabbedButtonBar().getNumTabs() - 1);
  add_hover_listener(*tab_button, module);
}

Component&
plugin_gui::make_modules(module_desc const* slots)
{
  int index = slots[0].module->info.index;
  auto const& tag = slots[0].module->info.tag;
  auto& result = make_tab_component(tag.id, tag.name, index);
  for (int i = 0; i < slots[0].module->info.slot_count; i++)
    add_component_tab(result, make_param_sections(slots[i]), slots[i].info.global, std::to_string(i + 1));
  if(slots[0].module->info.slot_count > 1)
    init_multi_tab_component(result, tag.id);
  return result;
}

Component&
plugin_gui::make_param_sections(module_desc const& module)
{
  auto const& topo = *module.module;
  auto& result = make_component<grid_component>(topo.gui.dimension, margin_section);
  for (int s = 0; s < topo.sections.size(); s++)
    result.add(make_param_section(module, topo.sections[s]), topo.sections[s].gui.position);
  add_hover_listener(result, module.info.global);
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
  if (param.param->gui.label.contents == gui_label_contents::none)
    return make_param_editor(module, param);
  return make_param_label_edit(module, param);
}

Component&
plugin_gui::make_multi_param(module_desc const& module, param_desc const* slots)
{
  auto const& param = slots[0].param;
  bool vertical = param->gui.layout == param_layout::vertical;
  auto& result = make_component<grid_component>(vertical, param->info.slot_count, margin_param);
  for (int i = 0; i < param->info.slot_count; i++)
    result.add(make_single_param(module, slots[i]), vertical, i);
  return result;
}

Component&
plugin_gui::make_param_section(module_desc const& module, param_section const& section)
{
  auto const& params = module.params;
  grid_component& grid = make_component<param_section_grid>(this, &module, &section, margin_param);
  for (auto iter = params.begin(); iter != params.end(); iter += iter->param->info.slot_count)
    if(iter->param->gui.edit_type != gui_edit_type::none && iter->param->gui.section == section.index)
      grid.add(make_params(module, &(*iter)), iter->param->gui.position);
  
  auto outline1 = module.module->gui.colors.section_outline1;
  auto outline2 = module.module->gui.colors.section_outline2;
  int radius = _gui_state->desc().plugin->gui.section_corner_radius;
  if(section.gui.scroll_mode == gui_scroll_mode::none)
    return make_component<rounded_container>(&grid, radius, false, false, outline1, outline2);
  
  auto& viewer = make_component<autofit_viewport>(module_lnf(module.module->info.index));
  viewer.setViewedComponent(&grid, false);
  viewer.setScrollBarsShown(true, false);
  return make_component<rounded_container>(&viewer, radius, false, false, outline1, outline2);
}

Component&
plugin_gui::make_module_section(module_section_gui const& section)
{
  auto const& modules = _gui_state->desc().modules;
  if (!section.tabbed)
  {
    auto& grid = make_component<grid_component>(section.dimension, margin_module);
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
  auto& tabs = make_tab_component(section.id, section.tab_header, matched_module);
  for(int o = 0; o < section.tab_order.size(); o++)
    for (auto iter = modules.begin(); iter != modules.end(); iter += iter->module->info.slot_count)
      if (iter->module->gui.visible && iter->module->gui.section == section.index && section.tab_order[o] == iter->module->info.index)
        add_component_tab(tabs, make_param_sections(*iter), iter->info.global, iter->module->gui.tabbed_name);
  init_multi_tab_component(tabs, section.id);
  return tabs;
}

Component&
plugin_gui::make_param_label(module_desc const& module, param_desc const& param, gui_label_contents contents)
{
  Label* result = {};
  switch (contents)
  {
  case gui_label_contents::name:
    result = &make_component<param_name_label>(this, &module, 
      &param, _module_lnfs[module.module->info.index].get());
    break;
  case gui_label_contents::both:
  case gui_label_contents::value:
    result = &make_component<param_value_label>(this, &module, &param, 
      contents == gui_label_contents::both, _module_lnfs[module.module->info.index].get());
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
  if(param.param->gui.edit_type == gui_edit_type::output)
  {
    auto& result = make_param_label(module, param, gui_label_contents::value);
    result.setColour(Label::ColourIds::textColourId, module.module->gui.colors.control_text);
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
    result = &make_component<param_toggle_button>(this, &module, &param); break;
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

  auto& result = make_component<grid_component>(dimension, margin_param);
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
    MessageBoxIconType::QuestionIcon, "Init patch", "Are you sure?");
  NativeMessageBox::showAsync(options, [this](int result) {
    if(result == 0)
    {
      _gui_state->init(state_init_type::default_);
      fire_state_loaded();
    }
  });
}

void
plugin_gui::clear_patch()
{
  auto options = MessageBoxOptions::makeOptionsOkCancel(
    MessageBoxIconType::QuestionIcon, "Clear patch", "Are you sure?");
  NativeMessageBox::showAsync(options, [this](int result) {
    if (result == 0)
    {
      _gui_state->init(state_init_type::minimal);
      fire_state_loaded();
    }
  });
}

void
plugin_gui::save_patch()
{
  int save_flags = FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
  FileChooser* chooser = new FileChooser("Save", File(), String("*.") + _gui_state->desc().plugin->extension, true, false, this);
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
  FileChooser* chooser = new FileChooser("Load", File(), String("*.") + _gui_state->desc().plugin->extension, true, false, this);
  chooser->launchAsync(load_flags, [this](FileChooser const& chooser) {
    auto path = chooser.getResult().getFullPathName();
    delete& chooser;
    if (path.length() != 0)
      load_patch(path.toStdString());
  });
}

void
plugin_gui::load_patch(std::string const& path)
{
  auto icon = MessageBoxIconType::WarningIcon;
  auto result = plugin_io_load_file_all(path, *_gui_state, *_extra_state);
  if (result.error.size())
  {
    auto options = MessageBoxOptions::makeOptionsOk(icon, "Error", result.error, String(), this);
    AlertWindow::showAsync(options, nullptr);
    return;
  }

  fire_state_loaded();
  if (result.warnings.size())
  {
    String warnings;
    for (int i = 0; i < result.warnings.size() && i < 5; i++)
      warnings += String(result.warnings[i]) + "\n";
    if (result.warnings.size() > 5)
      warnings += String(std::to_string(result.warnings.size() - 5)) + " more...\n";
    auto options = MessageBoxOptions::makeOptionsOk(icon, "Warning", warnings, String(), this);
    AlertWindow::showAsync(options, nullptr);
  }
}

}