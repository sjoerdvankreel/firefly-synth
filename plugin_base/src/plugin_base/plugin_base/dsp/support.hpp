#pragma once

#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/dsp/block/plugin.hpp>

#include <vector>

namespace plugin_base {

jarray<float, 1> const&
normalized_to_raw_into(
  plugin_block const& block, int m, int p, 
  jarray<float, 1> const& in, jarray<float, 1>& out);

jarray<float, 1> const&
sync_or_freq_into_scratch(
  plugin_block const& block, int module, int sync_p, 
  int freq_p, int num_p, int den_p, int scratch);

}
