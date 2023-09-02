#pragma once
#include <infernal.base/topo.hpp>
#include <string>

namespace infernal::base {

module_group_topo 
make_module_group(
  std::string const& id, std::string const& name, 
  int module_count, module_scope scope, module_output output);

param_topo 
make_param_toggle();

}
