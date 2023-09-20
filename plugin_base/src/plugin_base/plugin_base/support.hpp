#pragma once

#include <plugin_base/topo.hpp>
#include <string>

namespace plugin_base {

std::vector<std::string>
note_names();

section_topo
make_section(
  std::string const& name, int section,
  gui_position const& position, gui_dimension const& dimension);

module_topo
make_module(
  std::string const& id, std::string const& name,
  int slot_count, module_stage stage, module_output output,
  gui_layout layout, gui_position const& position, gui_dimension const& dimension);

// block-rate
param_topo
param_toggle(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_label_contents label_contents, bool default_,
  gui_layout layout, gui_position const& position);
param_topo
param_steps(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label_contents label_contents,
  int min, int max, int default_,
  gui_layout layout, gui_position const& position);
param_topo
param_items(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label_contents label_contents,
  std::vector<item_topo>&& items, std::string const& default_,
  gui_layout layout, gui_position const& position);
param_topo
param_names(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label_contents label_contents,
  std::vector<std::string> const& names, std::string const& default_,
  gui_layout layout, gui_position const& position);

// either block or accurate rate
param_topo
param_pct(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label_contents label_contents, param_rate rate, bool unit,
  double min, double max, double default_,
  gui_layout layout, gui_position const& position);
param_topo
param_linear(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label_contents label_contents, param_rate rate,
  double min, double max, double default_, std::string const& unit,
  gui_layout layout, gui_position const& position);
param_topo
param_log(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label_contents label_contents, param_rate rate,
  double min, double max, double default_, double midpoint, std::string const& unit,
  gui_layout layout, gui_position const& position);
}
