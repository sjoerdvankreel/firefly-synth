#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <string>

namespace plugin_base {

int
index_of_item_tag(std::vector<list_item> const& items, int tag);

topo_tag
make_topo_tag(std::string const& id, std::string const& name);
topo_info
make_topo_info(std::string const& id, std::string const& name, int index, int slot_count);
gui_label 
make_label(gui_label_contents contents, gui_label_align align, gui_label_justify justify);
inline gui_label 
make_label_default(gui_label_contents contents)
{ return make_label(contents, gui_label_align::left, gui_label_justify::center); }
inline gui_label 
make_label_none()
{ return make_label_default(gui_label_contents::none); }

section_topo
make_section(int index, topo_tag const& tag, section_topo_gui const& gui);
section_topo_gui
make_section_gui(gui_position const& position, gui_dimension const& dimension);

module_dsp 
make_module_dsp(module_stage stage, module_output output, int output_count);
module_topo
make_module(topo_info const& info, module_dsp const& dsp, module_topo_gui const& gui);
module_topo_gui
make_module_gui(gui_layout layout, gui_position const& position, gui_dimension const& dimension);

param_dsp
make_param_dsp(param_direction direction, param_rate rate, param_format format);
inline param_dsp
make_param_dsp_accurate(param_format format)
{ return make_param_dsp(param_direction::input, param_rate::accurate, format); }
inline param_dsp
make_param_dsp_block()
{ return make_param_dsp(param_direction::input, param_rate::block, param_format::plain); }
inline param_dsp
make_param_dsp_output()
{ return make_param_dsp(param_direction::output, param_rate::block, param_format::plain); }

param_domain
make_domain_toggle(bool default_);
param_domain
make_domain_step(int min, int max, int default_);
param_domain
make_domain_item(std::vector<list_item> const& items, std::string const& default_);
param_domain
make_domain_name(std::vector<std::string> const& names, std::string const& default_);
param_domain
make_domain_percentage(double min, double max, double default_, int precision, bool unit);
param_domain
make_domain_linear(double min, double max, double default_, int precision, std::string const& unit);
param_domain
make_domain_log(double min, double max, double default_, double midpoint, int precision, std::string const& unit);

param_topo
make_param(topo_info const& info, param_dsp const& dsp, param_domain const& domain, param_topo_gui const& gui);
param_topo_gui
make_param_gui(int section, gui_edit_type edit_type, gui_layout layout, gui_position position, gui_label label);
inline param_topo_gui
make_param_gui_single(int section, gui_edit_type edit_type, gui_position position, gui_label label)
{ return make_param_gui(section, edit_type, gui_layout::single, position, gui_label(label)); }

param_topo
param_steps(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  int min, int max, int default_,
  param_direction direction, gui_edit_type edit_type,
  gui_label_contents label_contents, gui_label_align label_align, gui_label_justify label_justify,
  gui_layout layout, gui_position const& position);
param_topo
param_names(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  std::vector<std::string> const& names, std::string const& default_,
  param_direction direction, gui_edit_type edit_type,
  gui_label_contents label_contents, gui_label_align label_align, gui_label_justify label_justify,
  gui_layout layout, gui_position const& position);

// either block or accurate rate
param_topo
param_pct(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  double min, double max, double default_, int precision,
  param_direction direction, param_rate rate, param_format format, bool unit, gui_edit_type edit_type,
  gui_label_contents label_contents, gui_label_align label_align, gui_label_justify label_justify,
  gui_layout layout, gui_position const& position);
param_topo
param_linear(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  double min, double max, double default_, int precision, std::string const& unit,
  param_direction direction, param_rate rate, param_format format, gui_edit_type edit_type,
  gui_label_contents label_contents, gui_label_align label_align, gui_label_justify label_justify,
  gui_layout layout, gui_position const& position);
param_topo
param_log(
  std::string const& id, std::string const& name, int index, int slot_count, int section,
  double min, double max, double default_, double midpoint, int precision, std::string const& unit,
  param_direction direction, param_rate rate, param_format format, gui_edit_type edit_type,
  gui_label_contents label_contents, gui_label_align label_align, gui_label_justify label_justify,
  gui_layout layout, gui_position const& position);
}
