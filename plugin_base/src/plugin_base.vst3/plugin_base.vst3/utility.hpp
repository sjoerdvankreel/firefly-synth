#pragma once

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>
#include <vector>

namespace plugin_base::vst3 {

Steinberg::FUID fuid_from_text(char const* text);
std::vector<char> load_ibstream(Steinberg::IBStream* stream);

}