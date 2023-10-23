#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/block/plugin.hpp>

#include <vector>

namespace plugin_base {

jarray<float, 1> const&
normalized_to_raw_into(
  plugin_block const& block, int m, int p, 
  jarray<float, 1> const& in, jarray<float, 1>& out);

template <class TransformTimesig>
jarray<float, 1> const&
sync_or_rate_into_scratch(
  plugin_block const& block, bool sync, int module, int rate_p,
  int num_p, int den_p, int scratch, TransformTimesig transform_timesig)
{
  auto& result = block.state.own_scratch[scratch];
  int num = block.state.own_block_automation[num_p][0].step();
  int den = block.state.own_block_automation[den_p][0].step();
  if (sync) std::fill(result.begin(), result.end(), transform_timesig(block.host.bpm, num, den));
  else normalized_to_raw_into(block, module, rate_p, block.state.own_accurate_automation[rate_p][0], result);
  return result;
}

jarray<float, 1> const&
sync_or_freq_into_scratch(
  plugin_block const& block, bool sync, int module, 
  int freq_p, int num_p, int den_p, int scratch)
{ return sync_or_rate_into_scratch(block, sync, module, freq_p, num_p, den_p, scratch, timesig_to_freq); }

jarray<float, 1> const&
sync_or_time_into_scratch(
  plugin_block const& block, bool sync, int module, 
  int time_p, int num_p, int den_p, int scratch)
{ return sync_or_rate_into_scratch(block, sync, module, time_p, num_p, den_p, scratch, timesig_to_time); }

}
