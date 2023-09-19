#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/array.hpp>

#include <pluginterfaces/base/ibstream.h>

namespace plugin_base::vst3 {

bool load_state(plugin_desc const& desc, Steinberg::IBStream* stream, jarray<plain_value, 4>& state);

}