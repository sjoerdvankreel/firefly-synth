#include <infernal.base/engine.hpp>

namespace infernal::base {

plugin_engine::
plugin_engine(plugin_topo&& topo) :
_topo(std::move(topo)), _desc(_topo), _dims(_topo)
{
  // reserve this much but allocate (on the audio thread!) if necessary
  // still seems better than dropping events
  int note_limit_guess = _topo.polyphony * 64;
  int block_events_guess = _desc.param_mappings.size();
  int accurate_events_guess = _desc.param_mappings.size() * 64;

  // init everything that is not frame-count dependent
  _host_block.common = &_common_block;
  _plugin_block.host = &_common_block;
  _state.init(_dims.module_param_counts);
  _module_engines.init(_dims.module_counts);
  _common_block.notes.reserve(note_limit_guess);
  _accurate_frames.resize(_desc.param_mappings.size());
  _host_block.block_events.reserve(block_events_guess);
  _host_block.accurate_events.reserve(accurate_events_guess);
  _plugin_block.block_automation.init(_dims.module_param_counts);
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
  // host calls this and should provide the current block values
  _host_block.common->bpm = 0;
  _host_block.common->frame_count = 0;
  _host_block.common->stream_time = 0;
  _host_block.common->audio_input = nullptr;
  _host_block.common->notes.clear();
  _host_block.audio_output = nullptr;
  _host_block.block_events.clear();
  _host_block.accurate_events.clear();
  return _host_block;
}

void
plugin_engine::deactivate()
{
  // drop frame-count dependent memory
  _sample_rate = 0;
  _mixdown_engine.reset();
  _host_block.block_events.clear();
  _host_block.accurate_events.clear();
  _plugin_block.module_cv = {};
  _plugin_block.module_audio = {};
  _plugin_block.accurate_automation = {};
  for(int g = 0; g < _topo.module_groups.size(); g++)
    for(int m = 0; m < _topo.module_groups[g].module_count; m++)
      _module_engines[g][m].reset();
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{  
  deactivate();
  _sample_rate = sample_rate;

  // init frame-count dependent memory
  plugin_frame_dims frame_dims(_topo, max_frame_count);
  _mixdown_engine = _topo.mixdown_factory(sample_rate, max_frame_count);
  _plugin_block.module_cv.init(frame_dims.module_cv_frame_counts);
  _plugin_block.module_audio.init(frame_dims.module_audio_frame_counts);
  _plugin_block.accurate_automation.init(frame_dims.module_accurate_frame_counts);
  for (int g = 0; g < _topo.module_groups.size(); g++)
    for (int m = 0; m < _topo.module_groups[g].module_count; m++)
      _module_engines[g][m] = _topo.module_groups[g].engine_factory(sample_rate, max_frame_count);
}

void 
plugin_engine::process()
{
  // clear host audio out
  for(int c = 0; c < 2; c++)
    std::fill(
      _host_block.audio_output[c], 
      _host_block.audio_output[c] + _common_block.frame_count,
      0.0f);

  for(int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for (int m = 0; m < group.module_count; m++)
      if(group.output == module_output::cv)
      {
        // clear module cv output
        auto& curve = _plugin_block.module_cv[g][m];
        std::fill(curve.begin(), curve.begin() + _common_block.frame_count, 0.0f);
      }
      else if (group.output == module_output::audio)
      {
        // clear module audio output
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
        // set per-block automation values to current values (automation may overwrite)
        if(group.params[p].rate == param_rate::block)
          _plugin_block.block_automation[g][m][p] = _state[g][m][p];
        else
        {
          // set accurate automation values to current values (automation may overwrite)
          // note: need to go from actual to normalized values as interpolation must be done as linear
          auto& automation = _plugin_block.accurate_automation[g][m][p];
          std::fill(
            automation.begin(), 
            automation.begin() + _common_block.frame_count, 
            param_value::from_real(_state[g][m][p].real).to_normalized(group.params[p]));
        }
      }
  }
    
  // process per-block automation values
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

  // process accurate automation values, this is a bit tricky as 
  // 1) events can come in at any sample position and
  // 2) interpolation needs to be done on normalized (linear) values and
  // 3) the host must (!) provide a value at the very end of the buffer if that event would have effect w.r.t. the next block
  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block.accurate_events.size(); e++)
  {
    auto const& event = _host_block.accurate_events[e];
    auto const& mapping = _desc.param_mappings[event.plugin_param_index];
    auto const& group = _topo.module_groups[mapping.group];
    auto const& param = group.params[mapping.param];

    // linear interpolation from previous to current value
    auto& curve = _plugin_block.accurate_automation[mapping.group][mapping.module][mapping.param];
    int prev_frame = _accurate_frames[event.plugin_param_index];
    float frame_count = event.frame_index - prev_frame;
    float range = event.normalized - curve[prev_frame];
    for(int f = prev_frame; f < event.frame_index; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / frame_count * range;

    // set new state to denormalized and update last event timestamp
    _state[mapping.group][mapping.module][mapping.param].real 
      = param_value::from_normalized(param, event.normalized).real;
    _accurate_frames[event.plugin_param_index] = event.frame_index;
  }

  // denormalize all interpolated curves even if they where not changed
  // this might be costly for log-scaled parameters
  for (int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for (int m = 0; m < group.module_count; m++)
      for (int p = 0; p < group.params.size(); p++)
      {
        auto const& param = group.params[p];
        if(param.rate == param_rate::accurate)
        {
          for(int f = 0; f < _common_block.frame_count; f++)
            _plugin_block.accurate_automation[g][m][p][f] = param_value::from_normalized(
              param, _plugin_block.accurate_automation[g][m][p][f]).real;
        }
      }
  }

  // run all modules in order
  // each module has access to previous modules audio and cv outputs
  _plugin_block.sample_rate = _sample_rate;
  for(int g = 0; g < _topo.module_groups.size(); g++)
  {
    auto const& group = _topo.module_groups[g];
    for(int m = 0; m < group.module_count; m++)
    {
      module_block module(
        _plugin_block.module_cv[g][m], 
        _plugin_block.module_audio[g][m], 
        _plugin_block.accurate_automation[g][m],
        _plugin_block.block_automation[g][m]);
      _module_engines[g][m]->process(_topo, _plugin_block, module);
    }
  }

  // mixer will combine module outputs into host audio out
  _mixdown_engine->process(_topo, _plugin_block, _host_block.audio_output);
}

}