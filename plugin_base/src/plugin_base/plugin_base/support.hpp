#pragma once
#include <plugin_base/topo.hpp>
#include <string>

namespace plugin_base {

std::vector<std::string>
note_names();

module_group_topo
make_module_group(
  std::string const& id, std::string const& name,
  int module_count, module_scope scope, module_output output);

// block-rate input
param_topo
param_toggle(
  std::string const& id, std::string const& name, int group, 
  param_text text, bool default_);
param_topo
param_steps(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text,
  int min, int max, int default_);
param_topo
param_items(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text,
  items_topo_factory items_factory, std::string const& default_);
param_topo
param_names(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text,
  std::vector<std::string> const& names, std::string const& default_);

// either block or accurate rate input
param_topo
param_percentage(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text, param_rate rate,
  double min, double max, double default_);
param_topo
param_linear(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text, param_rate rate,
  double min, double max, double default_, std::string const& unit);
param_topo
param_log(
  std::string const& id, std::string const& name, int group,
  param_display display, param_text text, param_rate rate,
  double min, double max, double default_, double midpoint, std::string const& unit);
}
