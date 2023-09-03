#pragma once
#include <infernal.base/topo.hpp>
#include <infernal.base/desc.hpp>
#include <infernal.base/jarray.hpp>
#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <infernal.base/block_host.hpp>
#include <infernal.base/block_common.hpp>
#include <infernal.base/block_plugin.hpp>
#include <memory>
#include <utility>

namespace infernal::base {

class module_engine abstract {
public:
  virtual void process(
    plugin_topo const& topo, 
    int module_index, 
    plugin_block const& block) = 0;
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
  std::vector<int> _accurate_frames = {};
  jarray2d<std::unique_ptr<module_engine>> _engines = {};

public:
  ~plugin_engine();
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(plugin_topo&& topo);

  plugin_topo const& topo() const { return _topo; }
  plugin_desc const& desc() const { return _desc; }

  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);
};

}
