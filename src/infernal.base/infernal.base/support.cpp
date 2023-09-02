#include <infernal.base/support.hpp>

namespace infernal::base {

module_group_topo 
make_module_group(std::string const& id, std::string const& name, int module_count, module_scope scope, module_output output)
{
  module_group_topo result;
  result.id = id;
  result.name = name;
  result.scope = scope;
  result.output = output;
  result.module_count = module_count;
  return result;
}

}