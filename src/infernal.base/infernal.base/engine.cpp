#include <infernal.base/engine.hpp>

namespace infernal::base {

plugin_engine::
plugin_engine(plugin_topo&& topo) :
_topo(std::move(topo)), _desc(_topo), _dims(_topo)
{
  _host_block.common = &_common_block;
  _plugin_block.host = &_common_block;
  _engines.init(_dims.module_counts);
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
        _state[g][m][p] = param_value::default_value(group.params[p]);
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
  for(int g = 0; g < _topo.module_groups.size(); g++)
    for(int m = 0; m < _topo.module_groups[g].module_count; m++)
      _engines[g][m].reset();
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
  for (int g = 0; g < _topo.module_groups.size(); g++)
    for (int m = 0; m < _topo.module_groups[g].module_count; m++)
      _engines[g][m] = _topo.module_groups[g].engine_factory(sample_rate, max_frame_count);
}

void 
plugin_engine::process()
{
  for(int c = 0; c < 2; c++)
    std::fill(
      _common_block.audio_output[c], 
      _common_block.audio_output[c] + _common_block.frame_count, 
      0.0f);

  for(int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for (int m = 0; m < group.module_count; m++)
      if(group.output == module_output::cv)
      {
        auto& curve = _plugin_block.module_cv[g][m];
        std::fill(curve.begin(), curve.begin() + _common_block.frame_count, 0.0f);
      }
      else if (group.output == module_output::audio)
      {
        auto& audio = _plugin_block.module_audio[g][m];
        for(int c = 0; c < 2; c++)
          std::fill(audio[c].begin(), audio[c].begin() + _common_block.frame_count, 0.0f);
      }
  }

  for (int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
      for(int p = 0; p < group.params.size(); p++)
      {
        if(group.params[p].config.rate == param_rate::block)
          _plugin_block.block_automation[g][m][p] = _state[g][m][p];
        else
        {
          auto& automation = _plugin_block.accurate_automation[g][m][p];
          std::fill(
            automation.begin(), 
            automation.begin() + _common_block.frame_count, 
            param_value::from_real(_state[g][m][p].real).to_normalized(group.params[p]));
        }
      }
  }
    
  for (int e = 0; e < _host_block.block_events.size(); e++)
  {
    auto const& event = _host_block.block_events[e];
    auto const& mapping = _desc.param_mappings[event.plugin_param_index];
    auto const& group = _topo.module_groups[mapping.group];
    auto const& param = group.params[mapping.param];
    param_value denormalized = param_value::from_normalized(param, event.normalized);
    _state[mapping.group][mapping.module][mapping.param] = denormalized;
    _plugin_block.block_automation[mapping.group][mapping.module][mapping.param] = denormalized;
  }

  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block.accurate_events.size(); e++)
  {
    auto const& event = _host_block.accurate_events[e];
    auto const& mapping = _desc.param_mappings[event.plugin_param_index];
    auto& curve = _plugin_block.accurate_automation[mapping.group][mapping.module][mapping.param];
    int prev_frame = _accurate_frames[event.plugin_param_index];
    float frame_count = event.frame_index - prev_frame;
    float range = event.normalized - curve[prev_frame];
    for(int f = prev_frame; f < event.frame_index; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / frame_count * range;
    _state[mapping.group][mapping.module][mapping.param].real = event.normalized;
    _accurate_frames[event.plugin_param_index] = event.frame_index;
  }

  for(int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
      _engines[g][m]->process(_topo, m, _plugin_block);
  }
}

}