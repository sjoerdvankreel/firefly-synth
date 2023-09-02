#include <infernal.base/engine.hpp>

namespace infernal::base {

plugin_engine::
plugin_engine(plugin_topo&& topo) :
_topo(std::move(topo)), _desc(_topo) 
{
  _host_block.common = &_common_block;
  _plugin_block.host = &_common_block;
}

host_block&
plugin_engine::prepare()
{
  _host_block.common->bpm = 0;
  _host_block.common->frame_count = 0;
  _host_block.common->stream_time = 0;
  _host_block.common->audio_input = nullptr;
  _host_block.common->audio_output = nullptr;
  _host_block.common->notes.clear();
  _host_block.block_events.clear();
  _host_block.accurate_events.clear();
  return _host_block;
}

void
plugin_engine::deactivate()
{
  _sample_rate = 0;
  _state = {};
  _accurate_frames = {};
  _host_block.block_events.clear();
  _host_block.accurate_events.clear();
  _plugin_block.module_cv = {};
  _plugin_block.module_audio = {};
  _plugin_block.block_automation = {};
  _plugin_block.accurate_automation = {};
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{
  std::vector<int> module_counts;
  std::vector<std::vector<int>> module_frame_counts;
  std::vector<std::vector<int>> module_param_counts;
  std::vector<std::vector<int>> module_channel_counts;
  std::vector<std::vector<std::vector<int>>> module_param_frame_counts;
  std::vector<std::vector<std::vector<int>>> module_channel_frame_counts;

  deactivate();
  

  _sample_rate = sample_rate;
  _accurate_frames.resize(max_frame_count);
  _state.init(group_count, module_counts, module_param_counts);
  _host_block.block_events.reserve(_topo.block_automation_limit);
  _host_block.accurate_events.reserve(_topo.accurate_automation_limit);
  _plugin_block.module_cv.init(group_count, module_counts, module_frame_counts);
  _plugin_block.block_automation.init(group_count, module_counts, module_param_counts);
  _plugin_block.module_audio.init(group_count, module_counts, module_channel_counts, module_channel_frame_counts);
  _plugin_block.accurate_automation.init(group_count, module_counts, module_param_counts, module_param_frame_counts);
}

void 
plugin_engine::process()
{
  for(int m = 0; m < _topo.static_topo.modules.size(); m++)
  {
    auto const& flat_mod = _topo.flat_modules[m].static_topo;
    if (flat_mod->output == module_output::cv)
      for(int i = 0; i < flat_mod->count; i++)
        std::fill(
          _plugin_block.module_cv[m][i], 
          _plugin_block.module_cv[m][i] + _host_block.common.frame_count,
          std::numeric_limits<float>::quiet_NaN());
    else if(flat_mod->output == module_output::audio)
      for (int i = 0; i < flat_mod->count; i++)
        for(int c = 0; c < _topo.static_topo.channel_count; c++)
          std::fill(
            _plugin_block.module_audio[m][i][c], 
            _plugin_block.module_audio[m][i][c] + _host_block.common.frame_count,
            std::numeric_limits<float>::quiet_NaN());
  }

  for(int p = 0; p < _topo.runtime_params.size(); p++)
  {
    auto const& rt_param = _topo.runtime_params[p];
    int mt = rt_param.module_type;
    int mi = rt_param.module_index;
    int pi = rt_param.module_param_index;
    int pt = rt_param.static_topo->type;
    if (_topo.flat_modules[mt].params[pi]->rate == param_rate::block)
      _plugin_block.block_automation[mt][mi][pt] = _state[mt][mi][pt];
    else
      std::fill(
        _plugin_block.accurate_automation[mt][mi][pt],
        _plugin_block.accurate_automation[mt][mi][pt] + _host_block.common.frame_count,
        _state[mt][mi][pt].real);
  }
    
  for (int a = 0; a < _host_block.block_automation.size(); a++)
  {
    auto const& event = _host_block.block_automation[a];
    auto const& rt_param = _topo.runtime_params[event.runtime_param_index];
    int mt = rt_param.module_type;
    int mi = rt_param.module_index;
    int pt = rt_param.static_topo->type;
    _state[mt][mi][pt] = event.value;
    _plugin_block.block_automation[mt][mi][pt] = event.value;
  }

  std::fill(_accurate_automation_frames.begin(), _accurate_automation_frames.end(), 0);
  for (int a = 0; a < _host_block.accurate_automation.size(); a++)
  {
    auto const& event = _host_block.accurate_automation[a];
    auto rpi = event.runtime_param_index;
    auto const& rt_param = _topo.runtime_params[rpi];
    int mt = rt_param.module_type;
    int mi = rt_param.module_index;
    int pt = rt_param.static_topo->type;
    int prev_frame = _accurate_automation_frames[rpi];
    float frame_count = event.frame_index - prev_frame;
    float start = _plugin_block.accurate_automation[mt][mi][pt][_accurate_automation_frames[rpi]];
    float range = event.value - start;
    for(int f = prev_frame; f < event.frame_index; f++)
      _plugin_block.accurate_automation[mt][mi][pt][f] = start + (f - prev_frame) / frame_count * range;
    _state[mt][mi][pt].real = event.value;
    _accurate_automation_frames[rpi] = event.frame_index;
  }

  for(int m = 0; m < _topo.static_topo.modules.size(); m++)
  {
    auto const& static_mod = _topo.static_topo.modules[m];
    for(int i = 0; i < static_mod.count; i++)
      static_mod.callbacks.process(_topo.static_topo, i, _plugin_block);
  }
}

}