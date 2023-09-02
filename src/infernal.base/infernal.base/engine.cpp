#include <infernal.base/engine.hpp>

namespace infernal::base {

plugin_engine::
plugin_engine(plugin_topo&& topo) :
_topo(std::move(topo)), _desc(_topo), _dims(_topo)
{
  _host_block.common = &_common_block;
  _plugin_block.host = &_common_block;
  _state.init(_dims.module_param_counts);
  _common_block.notes.reserve(_topo.note_limit);
  _plugin_block.block_automation.init(_dims.module_param_counts);
  _host_block.block_events.reserve(_topo.block_automation_limit);
  _host_block.accurate_events.reserve(_topo.accurate_automation_limit);
  for (int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
      for(int p = 0; p < group.params.size(); p++)
      { 
        auto const& param = group.params[p];
        _state[group.type][m][param.type] = param_value::default_value(param);
      }
  }
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
  _accurate_frames = {};
  _host_block.block_events.clear();
  _host_block.accurate_events.clear();
  _plugin_block.module_cv = {};
  _plugin_block.module_audio = {};
  _plugin_block.accurate_automation = {};
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{  
  deactivate();
  plugin_frame_dims frame_dims(_topo, max_frame_count);
  _sample_rate = sample_rate;
  _accurate_frames.resize(max_frame_count);
  _plugin_block.module_cv.init(frame_dims.module_cv_frame_counts);
  _plugin_block.module_audio.init(frame_dims.module_audio_frame_counts);
  _plugin_block.accurate_automation.init(frame_dims.module_accurate_frame_counts);
}

void 
plugin_engine::process()
{
  for (int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
      for(int p = 0; p < group.params.size(); p++)
      {
        auto const& param = group.params[p];
        if(group.params[p].rate == param_rate::block)
          _plugin_block.block_automation[group.type][m][param.type] 
            = _state[group.type][m][param.type];
        else
        {
          auto& automation = _plugin_block.accurate_automation[group.type][m][param.type];
          std::fill(
            automation.begin(), 
            automation.begin() + _host_block.common->frame_count, 
            param_value::from_real(_state[group.type][m][param.type].real).to_normalized(param));
        }
      }
  }
    
  for (int e = 0; e < _host_block.block_events.size(); e++)
  {
    auto const& event = _host_block.block_events[e];
    auto const& mapping = _desc.param_mappings[event.plugin_param_index];
    auto const& group = _topo.module_groups[mapping.group_index];
    auto const& param = group.params[mapping.param_index];
    param_value denormalized = param_value::from_normalized(param, event.normalized);
    _state[mapping.group_type][mapping.module_index][mapping.param_type] = denormalized;
    _plugin_block.block_automation[mapping.group_type][mapping.module_index][mapping.param_type] = denormalized;
  }

  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block.accurate_events.size(); e++)
  {
    auto const& event = _host_block.accurate_events[e];
    auto const& mapping = _desc.param_mappings[event.plugin_param_index];
    auto const& group = _topo.module_groups[mapping.group_index];
    auto const& param = group.params[mapping.param_index];
    auto& curve = _plugin_block.accurate_automation[mapping.group_type][mapping.module_index][mapping.param_type];
    int prev_frame = _accurate_frames[event.plugin_param_index];
    float frame_count = event.frame_index - prev_frame;
    float start = curve[prev_frame];
    float range = event.normalized - start;
    for(int f = prev_frame; f < event.frame_index; f++)
      curve[f] = start + (f - prev_frame) / frame_count * range;
    _state[mapping.group_type][mapping.module_index][mapping.param_type].real = event.normalized;
    _accurate_frames[event.plugin_param_index] = event.frame_index;
  }

  for(int m = 0; m < _topo.static_topo.modules.size(); m++)
  {
    auto const& static_mod = _topo.static_topo.modules[m];
    for(int i = 0; i < static_mod.count; i++)
      static_mod.callbacks.process(_topo.static_topo, i, _plugin_block);
  }
}

}