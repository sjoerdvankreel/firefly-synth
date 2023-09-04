#pragma once
#include <infernal.base/topo.hpp>

#define INF_SYNTH_NAME "Infernal Synth";
#define INF_SYNTH_VERSION_MAJOR 2
#define INF_SYNTH_VERSION_MINOR 0
#define INF_SYNTH_ID "{5F4B273A-391B-46D6-A300-56A5007D29C5}"

namespace infernal::synth {

enum module_type { module_type_osc, module_type_filter };

base::plugin_topo synth_topo();
base::module_group_topo osc_topo();
base::module_group_topo filter_topo();

}
