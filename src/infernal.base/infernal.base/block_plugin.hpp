#pragma once
#include <infernal.base/jarray.hpp>
#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>

namespace infernal::base {

struct common_block;

struct plugin_block final {
  float sample_rate;
  common_block const* host;
  jarray3d<float> module_cv;
  jarray4d<float> module_audio;
  jarray4d<float> accurate_automation;
  jarray3d<param_value> block_automation;
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

struct module_block final {
  int module_index;
  jarray2d<float>* audio_output;
  std::vector<float>* cv_output;
  jarray2d<float> const* accurate_automation;
  std::vector<param_value> const* block_automation;
  INF_DECLARE_MOVE_ONLY(module_block);
};

}