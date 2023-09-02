#pragma once
#include <infernal.base/topo.hpp>
#include <string>

namespace infernal::base {

module_group_topo 
make_module_group(
  std::string const& id, std::string const& name, 
  int module_count, module_scope scope, module_output output);

param_topo 
make_param_toggle(
  std::string const& id, std::string const& name,
  std::string const& default_, param_direction direction);

param_topo
make_param_step(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, param_direction direction, param_display display);

param_topo
make_param_pct(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, param_direction direction, param_rate rate, param_display display);

}
