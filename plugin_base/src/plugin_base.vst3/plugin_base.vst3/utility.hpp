#pragma once

#include <plugin_base/jarray.hpp>
#include <plugin_base/dsp/value.hpp>
#include <plugin_base/desc/plugin.hpp>

#include <pluginterfaces/base/ibstream.h>

namespace plugin_base::vst3 {

bool load_state(
  plugin_desc const& desc, 
  Steinberg::IBStream* stream, 
  jarray<plain_value, 4>& state);

}