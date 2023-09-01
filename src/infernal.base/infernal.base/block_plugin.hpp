#pragma once
#include <infernal.base/mdarray.hpp>
#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>

namespace infernal::base {

struct common_block;

struct plugin_block final {
  float sample_rate;
  common_block const* host;
  array3d<float> module_cv;
  array4d<float> module_audio;
  array4d<float> accurate_automation;
  array3d<param_value> block_automation;
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

}