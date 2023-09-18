#pragma once

#include <plugin_base/topo.hpp>

namespace infernal_synth {

enum { module_osc, module_filter, module_delay };

plugin_base::module_topo osc_topo();
plugin_base::module_topo delay_topo();
plugin_base::module_topo filter_topo(int osc_slot_count);
std::unique_ptr<plugin_base::plugin_topo> synth_topo();

}
