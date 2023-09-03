#pragma once
#include <infernal.base/topo.hpp>

namespace infernal::synth {

enum module_type { module_type_osc, module_type_filter, module_type_main };
base::plugin_topo synth_topo();

}
