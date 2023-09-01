#pragma once

namespace infernal::base {

struct common_block;
struct param_value final { union { float real; int step; }; };

struct plugin_block final {
  float sample_rate;
  common_block const* host;
  float* const* const* module_cv;
  float* const* const* const* module_audio;
  param_value const* const* const* block_automation;
  float const* const* const* const* accurate_automation;
};

}