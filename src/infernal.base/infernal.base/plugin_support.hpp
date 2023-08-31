#pragma once
#include <infernal.base/plugin_topo.hpp>

namespace infernal::base {

param_topo param_toggle();
param_topo param_real(param_rate rate, param_slope slope, param_display display, char const* unit);

}
