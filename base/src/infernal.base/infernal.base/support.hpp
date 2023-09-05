#pragma once
#include <infernal.base/topo.hpp>
#include <string>

namespace infernal::base {

std::vector<std::string>
note_names();

// block-rate input
param_topo
param_toggle(
  std::string const& id, std::string const& name, int group, 
  bool default_);
param_topo
param_steps(
  std::string const& id, std::string const& name, int group,
  param_display display, int min, int max, int default_);
param_topo
param_names(
  std::string const& id, std::string const& name, int group,
  param_display display, std::vector<std::string> const& names, std::string const& default_);
param_topo
param_items(
  std::string const& id, std::string const& name, int group,
  param_display display, std::vector<item_topo> const& items, std::string const& default_);

// either block or accurate rate input
param_topo
param_percentage(
  std::string const& id, std::string const& name, int group,
  param_display display, param_rate rate, double min, double max, double default_);
param_topo
param_linear(
  std::string const& id, std::string const& name, int group,
  param_display display, param_rate rate, double min, double max, double default_, std::string const& unit);
param_topo
param_log(
  std::string const& id, std::string const& name, int group,
  param_display display, param_rate rate, double min, double max, double default_, double midpoint, std::string const& unit);
}
