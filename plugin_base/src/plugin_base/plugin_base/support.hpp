#pragma once

#include <plugin_base/topo.hpp>
#include <string>

namespace plugin_base {

std::vector<std::string>
note_names();

int
index_of_item_tag(std::vector<item_topo> const& items, int tag);

section_topo
make_section(
  std::string const& name, int section,
  gui_position const& position, gui_dimension const& dimension);

module_topo
make_module(
  std::string const& id, std::string const& name, int slot_count, 
  module_stage stage, module_output output, int output_count,
  gui_layout layout, gui_position const& position, gui_dimension const& dimension);

// block-rate
param_topo
param_toggle(
  std::string const& id, std::string const& name, int slot_count, int section, 
  bool default_, 
  param_dir dir,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify, 
  gui_layout layout, gui_position const& position);
param_topo
param_steps(
  std::string const& id, std::string const& name, int slot_count, int section,
  int min, int max, int default_,
  param_dir dir, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify, 
  gui_layout layout, gui_position const& position);
param_topo
param_items(
  std::string const& id, std::string const& name, int slot_count, int section, 
  std::vector<item_topo>&& items, std::string const& default_,
  param_dir dir, param_edit edit, 
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify, 
  gui_layout layout, gui_position const& position);
param_topo
param_names(
  std::string const& id, std::string const& name, int slot_count, int section, 
  std::vector<std::string> const& names, std::string const& default_,
  param_dir dir, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify, 
  gui_layout layout, gui_position const& position);

// either block or accurate rate
param_topo
param_pct(
  std::string const& id, std::string const& name, int slot_count, int section, 
  double min, double max, double default_, int precision,
  param_dir dir, param_rate rate, param_format format, bool unit, param_edit edit, 
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify, 
  gui_layout layout, gui_position const& position);
param_topo
param_linear(
  std::string const& id, std::string const& name, int slot_count, int section, 
  double min, double max, double default_, int precision, std::string const& unit,
  param_dir dir, param_rate rate, param_format format, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position);
param_topo
param_log(
  std::string const& id, std::string const& name, int slot_count, int section, 
  double min, double max, double default_, double midpoint, int precision, std::string const& unit,
  param_dir dir, param_rate rate, param_format format, param_edit edit,
  param_label_contents label_contents, param_label_align label_align, param_label_justify label_justify,
  gui_layout layout, gui_position const& position);
}
