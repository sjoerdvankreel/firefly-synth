#pragma once

#include <plugin_base/topo.hpp>

namespace infernal_synth {

enum { module_lfo, module_env, module_cv_matrix, module_osc, module_filter, module_delay, module_count };

std::vector<plugin_base::item_topo>
cv_matrix_target_osc_params(
  plugin_base::module_topo const& osc_topo);
std::vector<plugin_base::item_topo>
cv_matrix_target_filter_params(
  plugin_base::module_topo const& filter_topo);

plugin_base::module_topo lfo_topo();
plugin_base::module_topo env_topo();
plugin_base::module_topo osc_topo();
plugin_base::module_topo delay_topo(int polyphony);
plugin_base::module_topo filter_topo(int osc_slot_count);
plugin_base::module_topo cv_matrix_topo(
  plugin_base::module_topo const& lfo_topo, 
  plugin_base::module_topo const& env_topo, 
  plugin_base::module_topo const& osc_topo, 
  plugin_base::module_topo const& filter_topo);
std::unique_ptr<plugin_base::plugin_topo> synth_topo();

}
