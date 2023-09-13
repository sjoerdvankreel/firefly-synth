#pragma once
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

namespace plugin_base {

struct common_block;

class module_output_values final {
  friend class audio_engine;
  module_topo const* const _module;
  jarray2d<plain_value>* const _values;
public:
  INF_DECLARE_MOVE_ONLY(module_output_values);
  module_output_values(module_topo const* module, jarray2d<plain_value>* values):
  _module(module), _values(values) {}
  
  void 
  set_value(int param_type, int param_index, plain_value plain) const
  {
    assert(param_index >= 0);
    assert(param_index < _module->params[param_type].count);
    assert(_module->params[param_type].direction == param_direction::output);
    (*_values)[param_type][param_index] = plain;
  }
};

// readonly modules audio and cv state, interpolated automation curves, host block
struct plugin_block final {
  float sample_rate;
  common_block const* host;
  jarray3d<float> module_cv;
  jarray4d<float> module_audio;
  jarray5d<float> accurate_automation;
  jarray4d<plain_value> block_automation;
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

// writeable single module audio and cv state, output parameters, readonly single module interpolated automation curves
// note - i really dont like reference members in structs, but pointers have awkward usage code for indexers: (*cv_output)[0]
struct module_block final {
  std::vector<float>& cv_output;
  jarray2d<float>& audio_output;
  module_output_values& output_values;
  jarray3d<float> const& accurate_automation;
  jarray2d<plain_value> const& block_automation;
  INF_DECLARE_MOVE_ONLY(module_block);
  module_block(
    std::vector<float>& cv_output, 
    jarray2d<float>& audio_output, 
    module_output_values& output_values,
    jarray3d<float> const& accurate_automation,
    jarray2d<plain_value> const& block_automation):
    cv_output(cv_output), 
    audio_output(audio_output),
    output_values(output_values),
    accurate_automation(accurate_automation), 
    block_automation(block_automation) {}
};

}