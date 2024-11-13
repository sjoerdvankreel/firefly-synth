#pragma once

#include <plugin_base/dsp/block/shared.hpp>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>
#include <vector>

namespace plugin_base::vst3 {

struct data_exchange_block_content
{
  // just ignore all outputs above this 
  // TODO check how this fares
  static inline std::size_t constexpr max_mod_outputs = 1024;
  std::size_t mod_output_count;
  modulation_output mod_outputs[max_mod_outputs];
};

Steinberg::FUID fuid_from_text(char const* text);
std::vector<char> load_ibstream(Steinberg::IBStream* stream);

}