#pragma once

#include <plugin_base/topo.hpp>

namespace infernal_synth {

enum { module_lfo, module_env, module_cv_matrix, module_osc, module_filter, module_delay, module_count };

enum { env_param_a, env_param_d, env_param_s, env_param_r };
enum { filter_param_on, filter_param_freq, filter_param_osc_gain };
enum { lfo_param_sync, lfo_param_rate, lfo_param_num, lfo_param_denom };
enum { delay_param_on, delay_param_out, delay_param_voices, delay_param_cpu, delay_param_threads };
enum { cv_matrix_param_on, cv_matrix_param_active, cv_matrix_param_source, cv_matrix_param_lfo_index, cv_matrix_param_env_index };
enum { osc_param_note, osc_param_oct, osc_param_cent, osc_param_on, osc_param_type, osc_param_gain, osc_param_bal, osc_param_sine_gain, osc_param_saw_gain };

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
