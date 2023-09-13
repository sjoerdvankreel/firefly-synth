#pragma once
#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_host.hpp>
#include <plugin_base/block_common.hpp>
#include <plugin_base/block_plugin.hpp>
#include <memory>
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
  jarray3d<plain_value> _state = {};
  std::int64_t _prev_output_time = {}; // Dont update output too often.
  std::vector<int> _accurate_frames = {}; // track accurate event frame positions per parameter
  jarray2d<std::unique_ptr<module_engine>> _module_engines = {};

public:
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(plugin_topo_factory factory);

  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);

  plugin_desc const& desc() const { return _desc; }
  // CLAP needs direct write access to the audio thread.
  jarray3d<plain_value>& state() { return _state; }
  jarray3d<plain_value> const& state() const { return _state; }
};

}
