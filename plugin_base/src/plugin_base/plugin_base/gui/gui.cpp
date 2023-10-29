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

static int const margin_param = 1;
static int const margin_module = 2;
static int const margin_section = 0;
static int const margin_container = 2;
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

plugin_gui::
plugin_gui(plugin_state* gui_state) :
_lnf(&gui_state->desc(), -1), _gui_state(gui_state)
{
  setOpaque(true);
  setLookAndFeel(&_lnf);
  auto const& topo = *gui_state->desc().plugin;
  add_and_make_visible(*this, make_container());
  setSize(topo.gui.default_width, topo.gui.default_width * topo.gui.aspect_ratio_height / topo.gui.aspect_ratio_width);
}

void
plugin_gui::gui_changed(int index, plain_value plain)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_changed(index, plain);
}

void
plugin_gui::gui_begin_changes(int index)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_begin_changes(index);
}

void
plugin_gui::gui_end_changes(int index)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_end_changes(index);
}

void
plugin_gui::gui_changing(int index, plain_value plain)
{
  if (_gui_state->desc().params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_changing(index, plain);
}

void
plugin_gui::remove_listener(gui_listener* listener)
{
  auto iter = std::find(_gui_listeners.begin(), _gui_listeners.end(), listener);
  if (iter != _gui_listeners.end()) _gui_listeners.erase(iter);
}

void
plugin_gui::fire_state_loaded()
{
  for(int i = 0; i < _gui_state->desc().param_count; i++)
    gui_changed(i, _gui_state->get_plain_at_index(i));
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
  auto& grid = make_component<grid_component>(gui_dimension({ gui_dimension::auto_size, 1 }, { 1 }), margin_container);
  grid.add(make_top_bar(), { 0, 0 });
  grid.add(make_content(), { 1, 0 });
  return make_component<margin_component>(&grid, BorderSize<int>(margin_container));
}

Component&
plugin_gui::make_content()
{
  auto const& topo = *_gui_state->desc().plugin;
  auto& result = make_component<grid_component>(topo.gui.dimension, margin_module);
  for(int s = 0; s < topo.gui.sections.size(); s++)
    result.add(make_module_section(topo.gui.sections[s]), topo.gui.sections[s].position);
  return result;
}

Component&
plugin_gui::make_module_section(module_section_gui const& section)
{
  auto const& modules = _gui_state->desc().modules;
  auto& result = make_component<grid_component>(section.dimension, margin_module);
  for (auto iter = modules.begin(); iter != modules.end(); iter += iter->module->info.slot_count)
    if(iter->module->gui.section == section.index)
      result.add(make_modules(&(*iter)), iter->module->gui.position);
  return result;
}

Component&
plugin_gui::make_modules(module_desc const* slots)
{
  int index = slots[0].module->info.index;
  auto const& topo = *_gui_state->desc().plugin;
  _module_lnfs[index] = std::make_unique<lnf>(&_gui_state->desc(), index);
  auto& result = make_component<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
  result.setOutline(0);
  result.setTabBarDepth(topo.gui.font_height + 4);
  result.getTabbedButtonBar().setTitle(slots[0].module->info.tag.name);
  result.setLookAndFeel(module_lnf(index));
  auto background1 = slots[0].module->gui.colors.tab_background1;
  auto background2 = slots[0].module->gui.colors.tab_background2;
  for (int i = 0; i < slots[0].module->info.slot_count; i++)
  {
    int radius = topo.gui.module_corner_radius;
    auto& corners = make_component<rounded_container>(&make_param_sections(slots[i]), radius, background1, background2);
    auto& margin_comp = make_component<margin_component>(&corners, BorderSize<int>(1, 0, 0, 0));
    result.addTab(std::to_string(i + 1), Colours::transparentBlack, &margin_comp, false);
  }
  return result;
}

Component&
plugin_gui::make_param_sections(module_desc const& module)
{
  auto const& topo = *module.module;
  auto& result = make_component<grid_component>(topo.gui.dimension, margin_section);
  for (int s = 0; s < topo.sections.size(); s++)
    result.add(make_param_section(module, topo.sections[s]), topo.sections[s].gui.position);
  return result;
}

Component&
plugin_gui::make_param_section(module_desc const& module, param_section const& section)
{
  grid_component* grid = nullptr;
  if (module.module->sections.size() == 1)
    grid = &make_component<param_section_grid>(this, &module, &section, margin_param);
  else
    grid = &make_component<grid_component>(section.gui.dimension, margin_param);

  auto const& params = module.params;
  for (auto iter = params.begin(); iter != params.end(); iter += iter->param->info.slot_count)
    if(iter->param->gui.section == section.index)
      grid->add(make_params(module, &(*iter)), iter->param->gui.position);
  
  if(module.module->sections.size() == 1)
    return *grid;

  auto& group = make_component<param_section_group>(this, &module, &section);
  group.setText(section.tag.name);
  auto& margin = make_component<margin_component>(grid, param_section_border);
  add_and_make_visible(group, margin);
  return group;
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
plugin_gui::make_param_label(module_desc const& module, param_desc const& param, gui_label_contents contents)
{
  Label* result = {};
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

Component&
plugin_gui::make_param_editor(module_desc const& module, param_desc const& param)
{
  if(param.param->dsp.direction == param_direction::output)
    return make_param_label(module, param, gui_label_contents::value);

  Component* result = nullptr;
  switch (param.param->gui.edit_type)
  {
  case gui_edit_type::knob:
  case gui_edit_type::hslider:
  case gui_edit_type::vslider:
    result = &make_component<param_slider>(this, &module, &param); break;
  case gui_edit_type::text:
    result = &make_component<param_textbox>(this, &module, &param); 
    dynamic_cast<param_textbox*>(result)->applyFontToAllText(_lnf.font());
    break;
  case gui_edit_type::toggle:
    result = &make_component<param_toggle_button>(this, &module, &param); break;
  case gui_edit_type::list:
  case gui_edit_type::autofit_list:
    result = &make_component<param_combobox>(this, &module, &param, _module_lnfs[module.module->info.index].get()); break;
  default:
    assert(false);
    return *((Component*)nullptr);
  }

  // don't touch state for input in case it is a ui-state-bound parameter
  if (param.param->dsp.direction == param_direction::output)
    result->setEnabled(false);

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

// TODO just for now.
// Give plugin more control over this.
Component&
plugin_gui::make_top_bar()
{
  auto& result = make_component<grid_component>(
    gui_dimension({ gui_dimension::auto_size }, 
    { gui_dimension::auto_size, gui_dimension::auto_size }), margin_module);

  auto& save = make_component<autofit_button>(&_lnf, "Save");
  result.add(save, { 0, 1 });
  save.onClick = [this]() {
    int flags = FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
    FileChooser* chooser = new FileChooser("Save", File(), String("*.") + _gui_state->desc().plugin->extension, true, false, this);
    chooser->launchAsync(flags, [this](FileChooser const& chooser) {
      auto path = chooser.getResult().getFullPathName();
      delete& chooser;
      if (path.length() == 0) return;
      plugin_io_save_file(path.toStdString(), *_gui_state);
    });};

  auto& load = make_component<autofit_button>(&_lnf, "Load");
  result.add(load, { 0, 0 });
  load.onClick = [this]() {
    int flags = FileBrowserComponent::openMode;
    FileChooser* chooser = new FileChooser("Load", File(), String("*.") + _gui_state->desc().plugin->extension, true, false, this);
    chooser->launchAsync(flags, [this](FileChooser const& chooser) {
      auto path = chooser.getResult().getFullPathName();
      delete &chooser;
      if(path.length() == 0) return;

      auto icon = MessageBoxIconType::WarningIcon;
      auto result = plugin_io_load_file(path.toStdString(), *_gui_state);
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