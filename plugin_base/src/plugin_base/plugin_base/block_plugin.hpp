#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

namespace plugin_base {

struct common_block;

class module_output_values final {
  friend class audio_engine;
  module_topo const* const _module;
  jarray<plain_value, 2>* const _values;
public:
  INF_DECLARE_MOVE_ONLY(module_output_values);
  module_output_values(module_topo const* module, jarray<plain_value, 2>* values):
  _module(module), _values(values) {}
  
  void 
  set(int topo_index, int slot_index, plain_value plain) const
  {
    assert(slot_index >= 0);
    assert(slot_index < _module->params[topo_index].slot_count);
    assert(_module->params[topo_index].dir == param_dir::output);
    (*_values)[topo_index][slot_index] = plain;
  }
};

// module output
struct plugin_out final {
  jarray<float, 3> cv = {};
  jarray<float, 4> audio = {};
  INF_DECLARE_MOVE_ONLY(plugin_out);
};

// automation input
struct automation_in final {
  jarray<float, 5> accurate = {};
  jarray<plain_value, 4> block = {};
  INF_DECLARE_MOVE_ONLY(automation_in);
};

// global process call in/out
struct plugin_block final {
  plugin_out out = {};
  automation_in in = {};
  float sample_rate = {};
  common_block const* host = {};
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

// module process call in/out
struct module_block final {
  jarray<float, 1>& cv_output;
  jarray<float, 2>& audio_output;
  module_output_values& output_values;
  jarray<float, 3> const& accurate_automation;
  jarray<plain_value, 2> const& block_automation;
  INF_DECLARE_MOVE_ONLY(module_block);
  module_block(
    jarray<float, 1>& cv_output, 
    jarray<float, 2>& audio_output, 
    module_output_values& output_values,
    jarray<float, 3> const& accurate_automation,
    jarray<plain_value, 2> const& block_automation):
    cv_output(cv_output), 
    audio_output(audio_output),
    output_values(output_values),
    accurate_automation(accurate_automation), 
    block_automation(block_automation) {}
};

}