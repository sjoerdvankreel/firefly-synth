#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>

#include <vector>

namespace infernal_synth {

struct cv_matrix_output
{
  plugin_base::jarray<std::vector<float> const*, 4> modulation;
};

enum { module_lfo, module_env, module_cv_matrix, module_osc, module_filter, module_delay, module_count };

plugin_base::module_topo lfo_topo();
plugin_base::module_topo env_topo();
plugin_base::module_topo osc_topo();
plugin_base::module_topo delay_topo(int polyphony);
plugin_base::module_topo filter_topo(int osc_slot_count);
plugin_base::module_topo cv_matrix_topo(
  std::vector<plugin_base::module_topo const*> const& sources, 
  std::vector<plugin_base::module_topo const*> const& targets);
std::unique_ptr<plugin_base::plugin_topo> synth_topo();

}
