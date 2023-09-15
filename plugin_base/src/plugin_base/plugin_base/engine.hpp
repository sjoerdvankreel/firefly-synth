#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/block.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

#include <memory>
#include <chrono>
#include <utility>

namespace plugin_base {

// module audio processor fills its own audio/cv output (if any)
// final module engine is responsible to fill host audio output
class module_engine {
public:
  virtual void 
  process(plugin_topo const& topo, plugin_block const& plugin, module_block& module) = 0;
};

class plugin_engine final {
  plugin_desc const _desc;
  plugin_dims const _dims; 
  float _sample_rate = {};
  host_block _host_block = {};
  plugin_block _plugin_block = {};
  common_block _common_block = {};
  jarray<plain_value, 4> _state = {};
  std::vector<int> _accurate_frames = {}; // track accurate event frame positions per parameter
  std::chrono::milliseconds _activated_at_ms = {}; // dont push output params too often
  jarray<std::unique_ptr<module_engine>, 2> _module_engines = {};

public:
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(std::unique_ptr<plugin_topo>&& topo);

  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);

  plugin_desc const& desc() const { return _desc; }
  // CLAP needs direct write access to the audio thread.
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }
};

}
