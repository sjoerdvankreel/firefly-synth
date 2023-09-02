#pragma once
#include <infernal.base/topo.hpp>
#include <infernal.base/utility.hpp>
#include <infernal.base/mdarray.hpp>
#include <infernal.base/block_host.hpp>
#include <infernal.base/block_common.hpp>
#include <infernal.base/block_plugin.hpp>

namespace infernal::base {

class plugin_engine final {   

  float _sample_rate = {};
  host_block _host_block = {};
  plugin_block _plugin_block = {};
  array3d<param_value> _state = {};
  runtime_plugin_topo const _topo;
  std::vector<int> _accurate_automation_frames = {};

public:
  ~plugin_engine();
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(plugin_topo&& topo);

  void process();
  void deactivate();
  host_block& prepare();
  void activate(int sample_rate, int max_frame_count);
  runtime_plugin_topo const& topo() const { return _topo; }
};

}
