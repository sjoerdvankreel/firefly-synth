#pragma once
#include <plugin_base/topo.hpp>
#include <plugin_base/desc.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/param_value.hpp>
#include <plugin_base/block_host.hpp>
#include <plugin_base/block_common.hpp>
#include <plugin_base/block_plugin.hpp>
#include <memory>
#include <utility>

namespace infernal::base {

// single module audio processor fills its own output if any
class module_engine {
public:
  virtual void 
  process(plugin_topo const& topo, plugin_block const& plugin, module_block& module) = 0;
};

// plugin level audio processor combines module audio outputs into host audio output
class mixdown_engine {
public:
  virtual void
  process(plugin_topo const& topo, plugin_block const& plugin, float* const* mixdown) = 0;
};

class plugin_engine final {
  plugin_topo const _topo;
  plugin_desc const _desc;
  plugin_dims const _dims; 
  float _sample_rate = {};
  host_block _host_block = {};
  plugin_block _plugin_block = {};
  common_block _common_block = {};
  jarray3d<param_value> _state = {};
  // these keep track of accurate event frame positions per parameter
  std::vector<int> _accurate_frames = {};
  std::unique_ptr<mixdown_engine> _mixdown_engine = {};
  jarray2d<std::unique_ptr<module_engine>> _module_engines = {};

public:
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(plugin_topo&& topo);  

  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);

  plugin_topo const& topo() const { return _topo; }
  plugin_desc const& desc() const { return _desc; }
  jarray3d<param_value> const& state() const { return _state; }
};

}
