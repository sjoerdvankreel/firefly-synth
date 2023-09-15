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

// single module audio processor
class module_engine {
public:
  virtual void 
  process(
    plugin_topo const& topo, 
    plugin_block const& plugin, 
    module_block& module) = 0;
};

// global plugin audio processor
class plugin_engine final {
  plugin_desc const _desc;
  plugin_dims const _dims; 
  float _sample_rate = {};
  host_block _host_block = {};
  plugin_block _plugin_block = {};
  common_block _common_block = {};
  jarray<plain_value, 4> _state = {};
  std::vector<int> _accurate_frames = {};
  std::chrono::milliseconds _activated_at_ms = {};
  jarray<std::unique_ptr<module_engine>, 2> _module_engines = {};

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
