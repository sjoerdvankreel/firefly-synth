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

// all modules cv/audio
struct plugin_out final {
  jarray<float, 3> cv = {};
  jarray<float, 4> audio = {};
  INF_DECLARE_MOVE_ONLY(plugin_out);
};

// all modules automation
struct plugin_in final {
  jarray<float, 5> accurate = {};
  jarray<plain_value, 4> block = {};
  INF_DECLARE_MOVE_ONLY(plugin_in);
};

// global process call in/out
struct plugin_block final {
  plugin_in in = {};
  plugin_out out = {};
  float sample_rate = {};
  common_block const* host = {};
  INF_DECLARE_MOVE_ONLY(plugin_block);
};

// single module cv/audio
struct module_out final {
  jarray<float, 1>* cv_ = {};
  jarray<float, 2>* audio_ = {};
  jarray<float, 1>& cv() const { return *cv_; }
  jarray<float, 2>& audio() const { return *audio_; }
  INF_DECLARE_MOVE_ONLY(module_out);
};

// single module automation
struct module_in final {
  jarray<float, 3> const* accurate_ = {};
  jarray<plain_value, 2> const* block_ = {};
  jarray<float, 3> const& accurate() { return *accurate_; }
  jarray<plain_value, 2> const& block() { return *block_; }
  INF_DECLARE_MOVE_ONLY(module_in);
};

// single module process call in/out
struct module_block final {
  module_in in;
  module_out out;
  module_output_values& output_values;
  INF_DECLARE_MOVE_ONLY(module_block);
};

}