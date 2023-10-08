#pragma once

#include <plugin_base/dsp/value.hpp>
#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>

#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ibstream.h>

namespace plugin_base::vst3 {

Steinberg::FUID
fuid_from_text(char const* text);

bool 
load_state(
  plugin_desc const& desc, 
  Steinberg::IBStream* stream, 
  jarray<plain_value, 4>& state);

}