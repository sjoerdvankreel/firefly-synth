#pragma once
#include <infernal.base/topo.hpp>
#include <string>

namespace infernal::base {

std::vector<item_topo>
note_name_items();

module_group_topo 
make_module_group(
  std::string const& id, std::string const& name, 
  int module_count, module_scope scope, module_output output);

param_topo 
make_param_input_toggle(
  std::string const& id, std::string const& name, std::string const& default_);
param_topo
make_param_input_step(
  std::string const& id, std::string const& name, std::string const& default_,
  int min, int max, param_display display);
param_topo
make_param_input_list(
  std::string const& id, std::string const& name, std::string const& default_,
  std::vector<item_topo> const& items, param_display display);
param_topo
make_param_input_pct(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, param_rate rate, param_display display);

}
