#pragma once

#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/block/plugin.hpp>
#include <vector>

namespace plugin_base {

inline timesig
get_timesig_param_value(plugin_block const& block, int module, int timesig_p)
{
  int sig_index = block.state.own_block_automation[timesig_p][0].step();
  return block.plugin.modules[module].params[timesig_p].domain.timesigs[sig_index];
}

inline float 
get_timesig_time_value(plugin_block const& block, float bpm, int module, int timesig_p)
{
  timesig sig = get_timesig_param_value(block, module, timesig_p);
  return timesig_to_time(bpm, sig);
}

inline float
get_timesig_freq_value(plugin_block const& block, int module, int timesig_p)
{
  timesig sig = get_timesig_param_value(block, module, timesig_p);
  return timesig_to_freq(block.host.bpm, sig);
}

template <class TransformTimesig>
float sync_or_rate_from_state(
  plugin_state const& state, float bpm, bool sync, int module_index, 
  int module_slot, int rate_p, int timesig_p, TransformTimesig transform_timesig)
{
  if (!sync) return state.get_raw_at(module_index, module_slot, rate_p, 0);
  int sig_index = state.get_plain_at(module_index, module_slot, timesig_p, 0).step();
  timesig sig = state.desc().plugin->modules[module_index].params[timesig_p].domain.timesigs[sig_index];
  return transform_timesig(bpm, sig);
}

inline float
sync_or_time_from_state(
  plugin_state const& state, float bpm, bool sync, int module_index, int module_slot, int time_p, int timesig_p)
{ return sync_or_rate_from_state(state, bpm, sync, module_index, module_slot, time_p, timesig_p, timesig_to_time); }

inline float
sync_or_freq_from_state(
  plugin_state const& state, float bpm, bool sync, int module_index, int module_slot, int freq_p, int timesig_p)
{ return sync_or_rate_from_state(state, bpm, sync, module_index, module_slot, freq_p, timesig_p, timesig_to_freq); }

}
