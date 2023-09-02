#include <infernal.base/support.hpp>

namespace infernal::base {

module_group_topo 
make_module_group(
  std::string const& id, std::string const& name, 
  int module_count, module_scope scope, module_output output)
{
  module_group_topo result = {};
  result.id = id;
  result.name = name;
  result.scope = scope;
  result.output = output;
  result.module_count = module_count;
  return result;
}

param_topo
make_param_toggle(
  std::string const& id, std::string const& name,
  std::string const& default_, param_direction direction)
{
  param_topo result = {};
  result.min = 0;
  result.max = 1;
  result.id = id;
  result.name = name;
  result.direction = direction;
  result.default_text = default_;
  result.rate = param_rate::block;
  result.format = param_format::step;
  result.storage = param_storage::num;
  result.display = param_display::toggle;
  return result;
}

param_topo
make_param_step(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, param_direction direction, param_display display)
{
  param_topo result = {};
  result.id = id;
  result.max = max;
  result.min = min;
  result.unit = "";
  result.name = name;
  result.display = display;
  result.direction = direction;
  result.default_text = default_;
  result.rate = param_rate::block;
  result.format = param_format::step;
  result.storage = param_storage::num;
  return result;
}

param_topo
make_param_pct(
  std::string const& id, std::string const& name, std::string const& default_,
  double min, double max, param_direction direction, param_rate rate, param_display display)
{
  param_topo result = {};
  result.id = id;
  result.max = max;
  result.min = min;
  result.unit = "%";
  result.name = name;
  result.rate = rate;
  result.percentage = true;
  result.display = display;
  result.direction = direction;
  result.default_text = default_;
  result.storage = param_storage::num;
  result.format = param_format::linear;
  return result;
}

}