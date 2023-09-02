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

}