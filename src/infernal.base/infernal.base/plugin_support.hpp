#pragma once
#include <infernal.base/plugin_topo.hpp>

namespace infernal::base {

param_topo param_input_block_toggle(int type, char const* id, char const* name, char const* default_);
param_topo param_input_accurate_linear(char const* id, char const* name, char const* default_, int type, char const* unit, double min, double max);

}
