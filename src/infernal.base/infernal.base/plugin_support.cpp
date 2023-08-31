#include <infernal.base/plugin_support.hpp>

namespace infernal::base {

static param_topo
param_poison()
{
  param_topo result;
  result.id = "\0";
  result.name = "\0";
  result.unit = "\0";
  result.default_ = "\0";
  result.type = -1;
  result.stepped_min = -1;
  result.stepped_max = -2;
  result.rate = (param_rate)-1;
  result.slope = (param_slope)-1;
  result.format = (param_format)-1;
  result.display = (param_display)-1;
  result.storage = (param_storage)-1;
  result.direction = (param_direction)-1;
  return result;
}

param_topo
param_toggle()
{
  param_topo result = param_poison();
  result.unit = "";
  result.stepped_min = 0;
  result.stepped_max = 1;
  result.rate = param_rate::block;
  result.slope = param_slope::linear;
  result.format = param_format::step;
  result.storage = param_storage::num;
  result.display = param_display::toggle;
  result.direction = param_direction::input;
  return result;
}

}