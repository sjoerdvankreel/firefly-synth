#include <infernal.base/plugin_support.hpp>

namespace infernal::base {

static param_topo
param_poison()
{
  param_topo result;
  result.min = -1;
  result.max = -2;
  result.type = -1;
  result.id = "\0";
  result.name = "\0";
  result.unit = "\0";
  result.default_ = "\0";
  result.rate = (param_rate)-1;
  result.display = (param_display)-1;
  result.storage = (param_storage)-1;
  result.direction = (param_direction)-1;
  return result;
}

param_topo
param_toggle()
{
  param_topo result = param_poison();
  result.min = 0;
  result.max = 1;
  result.unit = "";
  result.rate = param_rate::block;
  result.format = param_format::step;
  result.storage = param_storage::num;
  result.display = param_display::toggle;
  result.direction = param_direction::input;
  return result;
}

}