#pragma once
#include <plugin_base/topo.hpp>
#include <string>

namespace plugin_base {

std::vector<std::string>
note_names();

module_topo
make_module(
  std::string const& id, std::string const& name,
  int slot_count, module_scope scope, module_output output);

// block-rate
param_topo
param_toggle(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_label label, bool default_);
param_topo
param_steps(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label,
  int min, int max, int default_);
param_topo
param_items(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label,
  std::vector<item_topo>&& items, std::string const& default_);
param_topo
param_names(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label,
  std::vector<std::string> const& names, std::string const& default_);

// either block or accurate rate
param_topo
param_pct(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label, param_rate rate, bool unit,
  double min, double max, double default_);
param_topo
param_linear(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label, param_rate rate,
  double min, double max, double default_, std::string const& unit);
param_topo
param_log(
  std::string const& id, std::string const& name, int slot_count, int section,
  param_dir dir, param_edit edit, param_label label, param_rate rate,
  double min, double max, double default_, double midpoint, std::string const& unit);
}
