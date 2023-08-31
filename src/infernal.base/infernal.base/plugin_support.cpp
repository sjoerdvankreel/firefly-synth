#include <infernal.base/plugin_support.hpp>

namespace infernal::base {

param_topo 
param_input_block_toggle(char const* id, char const* name, char const* default_, int type)
{
  param_topo result;
  result.min = 0;
  result.max = 1;
  result.id = id;
  result.unit = "";
  result.type = type;
  result.name = name;
  result.default_ = default_;
  result.rate = param_rate::block;
  result.storage = param_storage::num;
  result.display = param_display::toggle;
  result.direction = param_direction::input;
  return result;
}

param_topo 
param_input_accurate_linear(char const* id, char const* name, char const* default_, int type, double min, double max)
{
  param_topo result;
  result.id = id;
  result.min = min;
  result.max = max;
  result.unit = "";
  result.type = type;
  result.name = name;
  result.default_ = default_;
  result.rate = param_rate::block;
  result.storage = param_storage::num;
  result.display = param_display::toggle;
  result.direction = param_direction::input;
  return result;
}

}