#pragma once
#include <infernal.base/topo.hpp>

namespace infernal::synth {

enum module_type { module_type_osc, module_type_filter };

module_group_topo osc_topo();
module_group_topo filter_topo();
base::plugin_topo synth_topo();

}
