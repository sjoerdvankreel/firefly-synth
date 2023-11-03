#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/block/plugin.hpp>
#include <vector>

namespace plugin_base {

jarray<float, 1> const&
normalized_to_raw_into(
  plugin_block const& block, int m, int p,
  jarray<float, 1> const& in, jarray<float, 1>& out)
{
  auto normalized_to_raw = [&block, m, p](float v) { return block.normalized_to_raw(m, p, v); };
  in.transform_to(block.start_frame, block.end_frame, out, normalized_to_raw);
  return out;
}

template <class TransformTimesig>
jarray<float, 1> const&
sync_or_rate_into_scratch(
  plugin_block const& block, bool sync, int module, int rate_p,
  int timesig_p, int scratch, TransformTimesig transform_timesig)
{
  auto& result = block.state.own_scratch[scratch];
  int sig_index = block.state.own_block_automation[timesig_p][0].step();
  timesig sig = block.plugin.modules[module].params[timesig_p].domain.timesigs[sig_index];
  if (sync) result.fill(block.start_frame, block.end_frame, transform_timesig(block.host.bpm, sig));
  else normalized_to_raw_into(block, module, rate_p, block.state.own_accurate_automation[rate_p][0], result);
  return result;
}

inline jarray<float, 1> const&
sync_or_freq_into_scratch(
  plugin_block const& block, bool sync, int module, int freq_p, int timesig_p, int scratch)
{ return sync_or_rate_into_scratch(block, sync, module, freq_p, timesig_p, scratch, timesig_to_freq); }

inline jarray<float, 1> const&
sync_or_time_into_scratch(
  plugin_block const& block, bool sync, int module, int time_p, int timesig_p, int scratch)
{ return sync_or_rate_into_scratch(block, sync, module, time_p, timesig_p, scratch, timesig_to_time); }

}
