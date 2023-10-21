#include <plugin_base/dsp/support.hpp>
#include <algorithm>

namespace plugin_base {

jarray<float, 1> const&
normalized_to_raw_into(
  plugin_block const& block, int m, int p,
  jarray<float, 1> const& in, jarray<float, 1>& out)
{
  auto normalized_to_raw = [&block, m, p](float v) { return block.normalized_to_raw(m, p, v); };
  std::transform(in.cbegin(), in.cend(), out.begin(), normalized_to_raw);
  return out;
}  

jarray<float, 1> const&
sync_or_freq_into_scratch(
  plugin_block const& block, bool sync, int module, 
  int freq_p, int num_p, int den_p, int scratch)
{
  auto& result = block.state.own_scratch[scratch];
  int num = block.state.own_block_automation[num_p][0].step();
  int den = block.state.own_block_automation[den_p][0].step();
  if (sync) std::fill(result.begin(), result.end(), timesig_to_freq(block.host.bpm, num, den));
  else normalized_to_raw_into(block, module, freq_p, block.state.own_accurate_automation[freq_p][0], result);
  return result;
}

}
