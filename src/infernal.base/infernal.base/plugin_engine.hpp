#ifndef INFERNAL_BASE_PLUGIN_ENGINE_HPP
#define INFERNAL_BASE_PLUGIN_ENGINE_HPP

#include <infernal.base/plugin_topo.hpp>
#include <infernal.base/plugin_block.hpp>

namespace infernal::base {

class plugin_engine {  
  plugin_topo const _topo;
  runtime_plugin_topo const _runtime_topo;
  param_value*** _state = {};
  host_block _host_block = {};
  plugin_block _plugin_block = {};
  std::vector<int> _accurate_automation_frames = {};

public:
  void process();
  void deactivate();
  host_block& prepare();
  void activate(int frame_count);

protected:
  ~plugin_engine();
  plugin_engine(plugin_topo const& topo);
};

}
#endif