#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_plugin.hpp>

#include <memory>
#include <chrono>
#include <utility>

namespace plugin_base {

struct host_block;

// single module audio processors
class module_engine { 
public: 
  virtual void process(process_block& block) = 0;
};

// global plugin audio processor
class plugin_engine final {

  plugin_desc const _desc;
  plugin_dims const _dims; 
  float _sample_rate = {};
  common_block _common_block = {};
  jarray<plain_value, 4> _state = {};
  std::vector<int> _accurate_frames = {};
  jarray<float, 3> _voices_results = {};
  jarray<float, 4> _voice_cv_state = {};
  jarray<float, 3> _global_cv_state = {};
  jarray<float, 5> _voice_audio_state = {};
  jarray<float, 4> _global_audio_state = {};
  jarray<float, 5> _accurate_automation = {};
  jarray<plain_value, 4> _block_automation = {};
  std::vector<voice_state> _voice_states = {};
  std::unique_ptr<host_block> _host_block = {};
  std::chrono::milliseconds _activated_at_ms = {};
  jarray<std::unique_ptr<module_engine>, 3> _voice_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _input_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _output_engines = {};

public:
  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);

  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(std::unique_ptr<plugin_topo>&& topo);

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }
};

}
