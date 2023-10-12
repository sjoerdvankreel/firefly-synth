#pragma once

#include <plugin_base/shared/state.hpp>

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>

namespace plugin_base::vst3 {

Steinberg::FUID fuid_from_text(char const* text);
bool load_state(Steinberg::IBStream* stream, plugin_state& state);

}