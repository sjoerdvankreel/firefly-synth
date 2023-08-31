#include <infernal.base/plugin_support.hpp>

namespace infernal::base {

param_topo
toggle_topo()
{
  param_topo result;
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