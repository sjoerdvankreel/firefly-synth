#pragma once
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

namespace plugin_base {

struct common_block;

// readonly modules audio and cv state, interpolated automation curves, host block
struct plugin_block final {
  float sample_rate;
  common_block const* host;
  jarray3d<float> module_cv;
  jarray4d<float> module_audio;
  jarray4d<float> accurate_automation;
  jarray3d<plain_value> block_automation;
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

// writeable single module audio and cv state, readonly single module interpolated automation curves
struct module_block final {
  std::vector<float>& cv_output;
  jarray2d<float>& audio_output;
  jarray2d<float> const& accurate_automation;
  std::vector<plain_value> const& block_automation;
  INF_DECLARE_MOVE_ONLY(module_block);
  module_block(
    std::vector<float>& cv_output, jarray2d<float>& audio_output,
    jarray2d<float> const& accurate_automation, std::vector<plain_value> const& block_automation):
    cv_output(cv_output), audio_output(audio_output),
    accurate_automation(accurate_automation), block_automation(block_automation) {}
};

}