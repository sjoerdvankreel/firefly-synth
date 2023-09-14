#include <plugin_base/engine.hpp>

namespace plugin_base {

plugin_engine::
plugin_engine(std::unique_ptr<plugin_topo>&& topo) :
_desc(std::move(topo)), _dims(*_desc.topo)
{
  // reserve this much but allocate (on the audio thread!) if necessary
  // still seems better than dropping events
  int note_limit_guess = _desc.topo->polyphony * 64;
  int block_events_guess = _desc.param_global_count;
  int accurate_events_guess = _desc.param_global_count * 64;

  // init everything that is not frame-count dependent
  _host_block.common = &_common_block;
  _plugin_block.host = &_common_block;
  _state.init(_dims.params);
  _desc.init_default_state(_state);
  _module_engines.init(_dims.modules);
  _common_block.notes.reserve(note_limit_guess);
  _accurate_frames.resize(_desc.param_global_count);
  _plugin_block.block_automation.init(_dims.params);
  _host_block.block_events.reserve(block_events_guess);
  _host_block.output_events.reserve(block_events_guess);
  _host_block.accurate_events.reserve(accurate_events_guess);
}

host_block&
plugin_engine::prepare()
{
  // host calls this and should provide the current block values
  _host_block.common->bpm = 0;
  _host_block.common->frame_count = 0;
  _host_block.common->stream_time = 0;
  _host_block.common->notes.clear();
  _host_block.common->audio_input = nullptr;
  _host_block.common->audio_output = nullptr;
  _host_block.block_events.clear();
  _host_block.output_events.clear();
  _host_block.accurate_events.clear();
  return _host_block;
}

void
plugin_engine::deactivate()
{
  // drop frame-count dependent memory
  _sample_rate = 0;
  _activated_at_ms = {};
  _host_block.block_events.clear();
  _host_block.output_events.clear();
  _host_block.accurate_events.clear();
  _plugin_block.module_cv = {};
  _plugin_block.module_audio = {};
  _plugin_block.accurate_automation = {};
  for(int m = 0; m < _desc.topo->modules.size(); m++)
    for(int mi = 0; mi < _desc.topo->modules[m].slot_count; mi++)
      _module_engines[m][mi].reset();
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{  
  deactivate();
  _sample_rate = sample_rate;

  // set activation time
  auto now_ticks = std::chrono::system_clock::now().time_since_epoch();
  _activated_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_ticks);

  // init frame-count dependent memory
  plugin_frame_dims frame_dims(*_desc.topo, max_frame_count);
  _plugin_block.module_cv.init(frame_dims.cv);
  _plugin_block.module_audio.init(frame_dims.audio);
  _plugin_block.accurate_automation.init(frame_dims.accurate);
  for (int m = 0; m < _desc.topo->modules.size(); m++)
    for (int mi = 0; mi < _desc.topo->modules[m].slot_count; mi++)
      _module_engines[m][mi] = _desc.topo->modules[m].engine_factory(sample_rate, max_frame_count);
}

void 
plugin_engine::process()
{
  // clear host audio out
  for(int c = 0; c < 2; c++)
    std::fill(
      _host_block.common->audio_output[c], 
      _host_block.common->audio_output[c] + _common_block.frame_count,
      0.0f);

  for(int m = 0; m < _desc.topo->modules.size(); m++)
  {
    auto const& module = _desc.topo->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      if(module.output == module_output::cv)
      {
        // clear module cv output
        auto& curve = _plugin_block.module_cv[m][mi];
        std::fill(curve.begin(), curve.begin() + _common_block.frame_count, 0.0f);
      }
      else if (module.output == module_output::audio)
      {
        // clear module audio output
        auto& audio = _plugin_block.module_audio[m][mi];
        for(int c = 0; c < 2; c++)
          std::fill(audio[c].begin(), audio[c].begin() + _common_block.frame_count, 0.0f);
      } else assert(module.output == module_output::none);
  }

  for (int m = 0; m < _desc.topo->modules.size(); m++)
  {
    auto const& module = _desc.topo->modules[m];
    for(int mi = 0; mi < module.slot_count; mi++)
      for(int p = 0; p < module.params.size(); p++)
      {
        // set per-block automation values to current values (automation may overwrite)
        auto const& param = module.params[p];
        if(param.rate == param_rate::block)
          for(int pi = 0; pi < param.slot_count; pi++)
          _plugin_block.block_automation[m][mi][p][pi] = _state[m][mi][p][pi];
        else
          for (int pi = 0; pi < param.slot_count; pi++)
          {
            // set accurate automation values to current values (automation may overwrite)
            // note: need to go from actual to normalized values as interpolation must be done as linear
            // later scale back from normalized to plain values
            auto& automation = _plugin_block.accurate_automation[m][mi][p][pi];
            std::fill(
              automation.begin(), 
              automation.begin() + _common_block.frame_count, 
              (float)param.raw_to_normalized(_state[m][mi][p][pi].real()).value());
        }
      }
  }
    
  // process per-block automation values
  for (int e = 0; e < _host_block.block_events.size(); e++)
  {
    auto const& event = _host_block.block_events[e];
    auto const& mapping = _desc.param_mappings[event.param_global_index];
    plain_value plain = _desc.param_at(mapping).topo->normalized_to_plain(event.normalized);
    mapping.value_at(_state) = plain;
    mapping.value_at(_plugin_block.block_automation) = plain;
  }

  // process accurate automation values, this is a bit tricky as 
  // 1) events can come in at any sample position and
  // 2) interpolation needs to be done on normalized (linear) values and
  // 3) the host must (!) provide a value at the very end of the buffer if that event would have effect w.r.t. the next block
  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block.accurate_events.size(); e++)
  {
    auto const& event = _host_block.accurate_events[e];
    auto const& mapping = _desc.param_mappings[event.param_global_index];

    // linear interpolation from previous to current value
    auto& curve = mapping.value_at(_plugin_block.accurate_automation);
    int prev_frame = _accurate_frames[event.param_global_index];
    float frame_count = event.frame_index - prev_frame + 1;
    float range = event.normalized.value() - curve[prev_frame];

    // note: really should be <=
    for(int f = prev_frame; f <= event.frame_index; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / frame_count * range;

    // set new state to denormalized and update last event timestamp
    mapping.value_at(_state).real_unchecked(_desc.param_at(mapping).topo->normalized_to_plain(event.normalized).real());
    _accurate_frames[event.param_global_index] = event.frame_index;
  }

  // denormalize all interpolated curves even if they where not changed
  // this might be costly for log-scaled parameters
  for (int m = 0; m < _desc.topo->modules.size(); m++)
  {
    auto const& module = _desc.topo->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for(int pi = 0; pi < param.slot_count; pi++)
          if(param.rate == param_rate::accurate)
            for(int f = 0; f < _common_block.frame_count; f++)
              _plugin_block.accurate_automation[m][mi][p][pi][f] = param.normalized_to_plain(
                normalized_value(_plugin_block.accurate_automation[m][mi][p][pi][f])).real();
      }
  }

  // run all modules in order
  // each module has access to previous modules audio and cv outputs
  _plugin_block.sample_rate = _sample_rate;
  for(int m = 0; m < _desc.topo->modules.size(); m++)
  {
    auto const& module = _desc.topo->modules[m];
    for(int mi = 0; mi < module.slot_count; mi++)
    {
      module_output_values output_values(&module, &_state[m][mi]);
      module_block block(
        _plugin_block.module_cv[m][mi], 
        _plugin_block.module_audio[m][mi], 
        output_values,
        _plugin_block.accurate_automation[m][mi],
        _plugin_block.block_automation[m][mi]);
      _module_engines[m][mi]->process(*_desc.topo, _plugin_block, block);
    }
  }

  // update output values 3 times a second
  _host_block.output_events.clear();
  auto now_ticks = std::chrono::system_clock::now().time_since_epoch();
  auto now_millis = std::chrono::duration_cast<std::chrono::milliseconds>(now_ticks);
  if((now_millis - _activated_at_ms).count() < 333) return;

  // push output events, we don't check if anything changed
  // just push events for all output parameters using the current value
  int global_param_index = 0;
  _activated_at_ms = now_millis;
  for (int m = 0; m < _desc.topo->modules.size(); m++)
  {
    auto const& module = _desc.topo->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for(int pi = 0; pi < param.slot_count; pi++)
        {
          if (param.dir == param_dir::output)
          {
            host_block_event output_event;
            output_event.param_global_index = global_param_index;
            output_event.normalized = param.plain_to_normalized(_state[m][mi][p][pi]);
            _host_block.output_events.push_back(output_event);
          }
          global_param_index++;
        }
      }
  }
}

}