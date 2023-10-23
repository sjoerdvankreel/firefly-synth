#include <plugin_base/helpers/dsp.hpp>
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

}
