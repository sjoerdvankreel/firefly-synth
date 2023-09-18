#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_common.hpp>

namespace plugin_base {

// single output module process call
struct output_module_block final {
  float* const* host_audio;
  jarray<plain_value, 2>& output_params;
  jarray<float, 2> const& voices_mixdown;
};

// single per-voice module process call
struct voice_module_block final {
  voice_state const& state;
  jarray<float, 2>& voice_result;
  jarray<float, 3> const& voice_cv_in;
  jarray<float, 4> const& voice_audio_in;
};

// single module process call
struct module_block {
  float sample_rate;
  common_block const& host;
  voice_module_block* voice;
  output_module_block* output;
  jarray<float, 1>& cv_out;
  jarray<float, 2>& audio_out;
  jarray<float, 3> const& global_cv_in;
  jarray<float, 4> const& global_audio_in;
  jarray<float, 3> const& accurate_automation;
  jarray<plain_value, 2> const& block_automation;
};


}