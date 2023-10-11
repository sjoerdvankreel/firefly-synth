#include <plugin_base/shared/io.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/controls.hpp>
#include <plugin_base/gui/containers.hpp>

#include <vector>
#include <algorithm>

using namespace juce;

namespace plugin_base {

static inline int constexpr label_width = 80;
static inline int constexpr label_height = 15;

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

plugin_gui::
plugin_gui(plugin_desc const* desc, jarray<plain_value, 4>* gui_state) :
_desc(desc), _gui_state(gui_state), _plugin_listeners(desc->param_count)
{
  setOpaque(true);
  auto const& topo = *_desc->plugin;
  addAndMakeVisible(&make_container());
  setSize(topo.gui.default_width, topo.gui.default_width * topo.gui.aspect_ratio_height / topo.gui.aspect_ratio_width);
}

void
plugin_gui::add_gui_listener(gui_listener* listener)
{
  _gui_listeners.push_back(listener);
}

void
plugin_gui::add_plugin_listener(int index, plugin_listener* listener)
{
  _plugin_listeners[index].push_back(listener);
}

void
plugin_gui::gui_changed(int index, plain_value plain)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_changed(index, plain);
}

void
plugin_gui::gui_begin_changes(int index)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_begin_changes(index);
}

void
plugin_gui::gui_end_changes(int index)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_end_changes(index);
}

void
plugin_gui::gui_changing(int index, plain_value plain)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_changing(index, plain);
}

void
plugin_gui::plugin_changed(int index, plain_value plain)
{
  for (int i = 0; i < _plugin_listeners[index].size(); i++)
    _plugin_listeners[index][i]->plugin_changed(index, plain);
}

void
plugin_gui::remove_gui_listener(gui_listener* listener)
{
  auto iter = std::find(_gui_listeners.begin(), _gui_listeners.end(), listener);
  if (iter != _gui_listeners.end()) _gui_listeners.erase(iter);
}

void
plugin_gui::remove_plugin_listener(int index, plugin_listener* listener)
{
  auto iter = std::find(_plugin_listeners[index].begin(), _plugin_listeners[index].end(), listener);
  if (iter != _plugin_listeners[index].end()) _plugin_listeners[index].erase(iter);
}

void
plugin_gui::fire_state_loaded()
{
  int param_global = 0;
  for (int m = 0; m < _desc->plugin->modules.size(); m++)
  {
    auto const& module = _desc->plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for (int pi = 0; pi < module.params[p].info.slot_count; pi++)
          gui_changed(param_global++, (*_gui_state)[m][mi][p][pi]);
  }
}

template <class T, class... U> T&
plugin_gui::make_component(U&&... args) 
{
  auto component = std::make_unique<T>(std::forward<U>(args)...);
  T* result = component.get();
  _components.emplace_back(std::move(component));
  return *result;
}

Component& 
plugin_gui::make_container()
{
  auto& result = make_component<grid_component>(gui_dimension({ -20, 1 }, { 1 }));
  result.add(make_top_bar(), { 0, 0 });
  result.add(make_content(), { 1, 0 });
  return result;
}

Component&
plugin_gui::make_content()
{
  auto& result = make_component<grid_component>(_desc->plugin->gui.dimension);
  for (auto iter = _desc->modules.begin(); iter != _desc->modules.end(); iter += iter->module->info.slot_count)
    result.add(make_modules(&(*iter)), iter->module->gui.position);
  return result;
}

Component&
plugin_gui::make_modules(module_desc const* slots)
{  
  if (slots[0].module->info.slot_count == 1)
    return make_single_module(slots[0], false);
  else
    return make_multi_module(slots);  
}

Component&
plugin_gui::make_single_module(module_desc const& slot, bool tabbed)
{
  if(tabbed) return make_sections(slot);
  auto& result = make_component<group_component>();
  result.setText(slot.info.name);
  result.addAndMakeVisible(make_sections(slot));
  return result;
}

Component&
plugin_gui::make_multi_module(module_desc const* slots)
{
  auto make_single = [this](module_desc const& m, bool tabbed) -> Component& {
    return make_single_module(m, tabbed); };
  return make_multi_slot(*slots[0].module, slots, make_single);
}

Component&
plugin_gui::make_sections(module_desc const& module)
{
  auto const& topo = *module.module;
  auto& result = make_component<grid_component>(topo.gui.dimension);
  for (int s = 0; s < topo.sections.size(); s++)
    result.add(make_section(module, topo.sections[s]), topo.sections[s].gui.position);
  return result;
}

Component&
plugin_gui::make_section(module_desc const& module, section_topo const& section)
{
  grid_component* grid = nullptr;
  if (module.module->sections.size() == 1)
    grid = &make_component<section_grid_component>(this, &module, &section);
  else
    grid = &make_component<grid_component>(section.gui.dimension);

  auto const& params = module.params;
  for (auto iter = params.begin(); iter != params.end(); iter += iter->param->info.slot_count)
    if(iter->param->gui.section == section.index)
      grid->add(make_params(module, &(*iter)), iter->param->gui.position);
  
  if(module.module->sections.size() == 1)
    return *grid;

  auto& result = make_component<section_group_component>(this, &module, &section);
  result.setText(section.tag.name);
  result.addAndMakeVisible(grid);
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
plugin_gui::make_multi_param(module_desc const& module, param_desc const* slots)
{
  auto make_single = [this, &module](param_desc const& p, bool tabbed) -> Component& {
    return make_single_param(module, p); };
  return make_multi_slot(*slots[0].param, slots, make_single);
}

Component& 
plugin_gui::make_param_label_edit(
  module_desc const& module, param_desc const& param, 
  gui_dimension const& dimension, gui_position const& label_position, gui_position const& edit_position)
{
  auto& result = make_component<grid_component>(dimension);
  result.add(make_param_label(module, param), label_position);
  result.add(make_param_editor(module, param), edit_position);
  return result;
}

Component&
plugin_gui::make_single_param(module_desc const& module, param_desc const& param)
{
  if(param.param->gui.label.contents == gui_label_contents::none)
    return make_param_editor(module, param);

  switch(param.param->gui.label.align)
  {
  case gui_label_align::top:
    return make_param_label_edit(module, param, gui_dimension({ -label_height, 1 }, { 1 }), { 0, 0 }, { 1, 0 });
  case gui_label_align::bottom:
    return make_param_label_edit(module, param, gui_dimension({ 1, -label_height, }, { 1 }), { 1, 0 }, { 0, 0 });
  case gui_label_align::left:
    return make_param_label_edit(module, param, gui_dimension({ 1 }, { -label_width, 1 }), { 0, 0 }, { 0, 1 });
  case gui_label_align::right:
    return make_param_label_edit(module, param, gui_dimension({ 1 }, { 1, -label_width }), { 0, 1 }, { 0, 0 });
  default:
    assert(false);
    return *((Component*)nullptr);
  }
}

Component&
plugin_gui::make_param_editor(module_desc const& module, param_desc const& param)
{
  Component* result = nullptr;
  switch (param.param->gui.edit_type)
  {
  case gui_edit_type::knob:
  case gui_edit_type::hslider:
  case gui_edit_type::vslider:
    result = &make_component<param_slider>(this, &module, &param); break;
  case gui_edit_type::text:
    result = &make_component<param_textbox>(this, &module, &param); break;
  case gui_edit_type::list:
    result = &make_component<param_combobox>(this, &module, &param); break;
  case gui_edit_type::dependent:
    result = &make_component<param_dependent>(this, &module, &param); break;
  case gui_edit_type::toggle:
    result = &make_component<param_toggle_button>(this, &module, &param); break;
  default:
    assert(false);
    return *((Component*)nullptr);
  }

  // don't touch state for input in case it is a ui-state-bound parameter
  if(param.param->dsp.direction == param_direction::output)
    result->setEnabled(false);

  return *result;
}

Component&
plugin_gui::make_param_label(module_desc const& module, param_desc const& param)
{
  Label* result = {};
  auto contents = param.param->gui.label.contents;
  switch (contents)
  {
  case gui_label_contents::name:
    result = &make_component<param_name_label>(this, &module, &param);
    break;
  case gui_label_contents::both:
  case gui_label_contents::value:
    result = &make_component<param_value_label>(this, &module, &param, contents == gui_label_contents::both);
    break;
  default:
    assert(false);
    break;
  }
  result->setJustificationType(justification_type(param.param->gui.label));
  return *result;
}

template <class Topo, class Slot, class MakeSingle>
Component&
plugin_gui::make_multi_slot(Topo const& topo, Slot const* slots, MakeSingle make_single)
{
  switch (topo.gui.layout)
  {
  case gui_layout::vertical:
  case gui_layout::horizontal:
  {
    bool vertical = topo.gui.layout == gui_layout::vertical;
    auto& result = make_component<grid_component>(vertical, topo.info.slot_count);
    for (int i = 0; i < topo.info.slot_count; i++)
      result.add(make_single(slots[i], false), vertical, i);
    return result;
  }
  case gui_layout::tabbed:
  {
    auto& result = make_component<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    for (int i = 0; i < topo.info.slot_count; i++)
      result.addTab(slots[i].info.name, Colours::black, &make_single(slots[i], true), false);
    return result;
  }
  default:
    assert(false);
    return *((Component*)nullptr);
  }
}

// TODO just for now.
// Give plugin more control over this.
Component&
plugin_gui::make_top_bar()
{
  auto& result = make_component<grid_component>(gui_dimension({ 1 }, { -100, -100 }));

  auto& save = make_component<TextButton>();
  save.setButtonText("Save");
  result.add(save, { 0, 1 });
  save.onClick = [this]() {
    int flags = FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
    FileChooser* chooser = new FileChooser("Save", File(), String("*.") + _desc->plugin->extension, true, false, this);
    chooser->launchAsync(flags, [this](FileChooser const& chooser) {
      auto path = chooser.getResult().getFullPathName();
      delete& chooser;
      if (path.length() == 0) return;
      plugin_io(_desc).save_file(path.toStdString(), *_gui_state);
    });};

  auto& load = make_component<TextButton>();
  load.setButtonText("Load");
  result.add(load, { 0, 0 });
  load.onClick = [this]() {
    int flags = FileBrowserComponent::openMode;
    FileChooser* chooser = new FileChooser("Load", File(), String("*.") + _desc->plugin->extension, true, false, this);
    chooser->launchAsync(flags, [this](FileChooser const& chooser) {
      auto path = chooser.getResult().getFullPathName();
      delete &chooser;
      if(path.length() == 0) return;

      auto icon = MessageBoxIconType::WarningIcon;
      auto result = plugin_io(_desc).load_file(path.toStdString(), *_gui_state);
      if(result.error.size())
      {
        auto options = MessageBoxOptions::makeOptionsOk(icon, "Error", result.error, String(), this);
        AlertWindow::showAsync(options, nullptr);
        return;
      }

      fire_state_loaded();
      if (result.warnings.size())
      {
        String warnings;
        for(int i = 0; i < result.warnings.size() && i < 5; i++)
          warnings += String(result.warnings[i]) + "\n";
        if(result.warnings.size() > 5)
          warnings += String(std::to_string(result.warnings.size() - 5)) + " more...\n";
        auto options = MessageBoxOptions::makeOptionsOk(icon, "Warning", warnings, String(), this);
        AlertWindow::showAsync(options, nullptr);
      }
    });};

  return result;
}

}