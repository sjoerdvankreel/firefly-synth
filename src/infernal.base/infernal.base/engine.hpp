#pragma once
#include <infernal.base/topo.hpp>
#include <infernal.base/utility.hpp>
#include <infernal.base/mdarray.hpp>
#include <infernal.base/block_host.hpp>
#include <infernal.base/block_common.hpp>
#include <infernal.base/block_plugin.hpp>
#include <utility>

namespace infernal::base {

class plugin_engine final {   

  plugin_topo const _topo;
  plugin_desc const _desc;
  float _sample_rate = {};
  host_block _host_block = {};
  plugin_block _plugin_block = {};
  array3d<param_value> _state = {};
  std::vector<int> _accurate_frames = {};

public:
  ~plugin_engine();
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(plugin_topo&& topo): 
  _topo(std::move(topo)), _desc(_topo) {}

  plugin_topo const& topo() const { return _topo; }
  plugin_desc const& desc() const { return _desc; }

  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);
};

}
