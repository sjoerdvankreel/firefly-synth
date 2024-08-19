#pragma once

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>
#include <vector>

namespace plugin_base::vst3 {

// fake these as output params to prevent having to deal with output events
inline int constexpr mod_indicator_output_param_count = 4096;
extern char const* mod_indicator_count_param_guid;
extern char const* mod_indicator_param_guids[mod_indicator_output_param_count];

Steinberg::FUID fuid_from_text(char const* text);
std::vector<char> load_ibstream(Steinberg::IBStream* stream);

}