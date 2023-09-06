#pragma once
#include <plugin_base/topo.hpp>

namespace infernal::synth {

enum module_type { module_type_osc, module_type_filter };

base::plugin_topo synth_topo();
base::module_group_topo osc_topo();
base::module_group_topo filter_topo();

}
