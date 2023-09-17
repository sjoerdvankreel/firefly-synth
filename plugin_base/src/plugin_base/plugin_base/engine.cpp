#include <plugin_base/engine.hpp>

namespace plugin_base {

plugin_engine::
plugin_engine(std::unique_ptr<plugin_topo>&& topo) :
_desc(std::move(topo)), _dims(*_desc.plugin)
{
  _host_block.common = &_common_block;
  _plugin_block.host = &_common_block;

  // reserve this much but allocate on the audio thread if necessary
  // still seems better than dropping events
  int block_events_guess = _desc.param_count;
  int accurate_events_guess = _desc.param_count * 64;
  int note_limit_guess = _desc.plugin->polyphony * 64;

  // init everything that is not frame-count dependent
  _state.resize(_dims.params);
  _desc.init_defaults(_state);
  _module_engines.resize(_dims.modules);
  _accurate_frames.resize(_desc.param_count);
  _plugin_block.in.block.resize(_dims.params);
  _common_block.notes.reserve(note_limit_guess);
  _host_block.events.out.reserve(block_events_guess);
  _host_block.events.block.reserve(block_events_guess);
  _host_block.events.accurate.reserve(accurate_events_guess);
}

host_block&
plugin_engine::prepare()
{
  // host calls this and should provide the current block values
  _host_block.events.out.clear();
  _host_block.events.block.clear();
  _host_block.events.accurate.clear();
  _host_block.common->notes.clear();
  _host_block.common->bpm = 0;
  _host_block.common->frame_count = 0;
  _host_block.common->stream_time = 0;
  _host_block.common->audio_in = nullptr;
  _host_block.common->audio_out = nullptr;
  return _host_block;
}

void
plugin_engine::deactivate()
{
  // drop frame-count dependent memory
  _sample_rate = 0;
  _activated_at_ms = {};
  _host_block.events.out.clear();
  _host_block.events.block.clear();
  _host_block.events.accurate.clear();
  _plugin_block.in.accurate = {};
  _plugin_block.out.voice_cv = {};
  _plugin_block.out.voice_audio = {};
  _plugin_block.out.global_cv = {};
  _plugin_block.out.global_audio = {};
  for(int m = 0; m < _desc.plugin->modules.size(); m++)
    for(int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
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
  plugin_frame_dims frame_dims(*_desc.plugin, max_frame_count);
  _plugin_block.out.voice_cv.resize(frame_dims.voice_cv);
  _plugin_block.out.voice_audio.resize(frame_dims.voice_audio);
  _plugin_block.out.global_cv.resize(frame_dims.global_cv);
  _plugin_block.out.global_audio.resize(frame_dims.global_audio);
  _plugin_block.in.accurate.resize(frame_dims.accurate);
  for (int m = 0; m < _desc.plugin->modules.size(); m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      _module_engines[m][mi] = _desc.plugin->modules[m].engine_factory(sample_rate, max_frame_count);
}

void 
plugin_engine::process()
{
  // TODO
  int voice_count = 2;

  // clear host audio out
  for(int c = 0; c < 2; c++)
    std::fill(
      _host_block.common->audio_out[c], 
      _host_block.common->audio_out[c] + _common_block.frame_count,
      0.0f);

  // clear module cv/audio out
  for(int m = 0; m < _desc.plugin->modules.size(); m++)
  {
    auto const& module = _desc.plugin->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      if(module.output == module_output::cv)
      {
        if(module.scope == module_scope::global)
        {
          auto& curve = _plugin_block.out.global_cv[m][mi];
          std::fill(curve.begin(), curve.begin() + _common_block.frame_count, 0.0f);
        } else for (int v = 0; v < voice_count; v++)
        {
          auto& curve = _plugin_block.out.voice_cv[v][m][mi];
          std::fill(curve.begin(), curve.begin() + _common_block.frame_count, 0.0f);
        }
      }
      else if (module.output == module_output::audio)
      {
        if (module.scope == module_scope::global)
        {
          auto& audio = _plugin_block.out.global_audio[m][mi];
          for(int c = 0; c < 2; c++)
            std::fill(audio[c].begin(), audio[c].begin() + _common_block.frame_count, 0.0f);
        } else for (int v = 0; v < voice_count; v++)
        {
          auto& audio = _plugin_block.out.voice_audio[v][m][mi];
          for (int c = 0; c < 2; c++)
            std::fill(audio[c].begin(), audio[c].begin() + _common_block.frame_count, 0.0f);;
        }
      } else assert(module.output == module_output::none);
  }

  // set automation values to state, automation may overwrite
  // accurate goes from plain to normalized as interpolation must be done in normalized
  for (int m = 0; m < _desc.plugin->modules.size(); m++)
  {
    auto const& module = _desc.plugin->modules[m];
    for(int mi = 0; mi < module.slot_count; mi++)
      for(int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        if(param.rate == param_rate::block)
          for(int pi = 0; pi < param.slot_count; pi++)
          _plugin_block.in.block[m][mi][p][pi] = _state[m][mi][p][pi];
        else
          for (int pi = 0; pi < param.slot_count; pi++)
          {
            auto& automation = _plugin_block.in.accurate[m][mi][p][pi];
            std::fill(
              automation.begin(), 
              automation.begin() + _common_block.frame_count, 
              (float)param.raw_to_normalized(_state[m][mi][p][pi].real()).value());
        }
      }
  }
    
  // process per-block automation values
  for (int e = 0; e < _host_block.events.block.size(); e++)
  {
    auto const& event = _host_block.events.block[e];
    auto const& mapping = _desc.mappings[event.param];
    plain_value plain = _desc.normalized_to_plain_at(mapping, event.normalized);
    mapping.value_at(_state) = plain;
    mapping.value_at(_plugin_block.in.block) = plain;
  }

  // process accurate automation values, this is a bit tricky as 
  // 1) events can come in at any frame position and
  // 2) interpolation needs to be done on normalized (linear) values and
  // 3) the host must provide a value at the end of the buffer if that event would have effect w.r.t. the next block
  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block.events.accurate.size(); e++)
  {
    // linear interpolate as normalized
    auto const& event = _host_block.events.accurate[e];
    auto const& mapping = _desc.mappings[event.param];
    auto& curve = mapping.value_at(_plugin_block.in.accurate);
    int prev_frame = _accurate_frames[event.param];
    float frame_count = event.frame - prev_frame + 1;
    float range = event.normalized.value() - curve[prev_frame];
    for(int f = prev_frame; f <= event.frame; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / frame_count * range;

    // denormalize
    mapping.value_at(_state).real_unchecked(_desc.normalized_to_plain_at(mapping, event.normalized).real());
    _accurate_frames[event.param] = event.frame;
  }

  // denormalize all interpolated curves even if they where not changed
  // this might be costly for log-scaled parameters
  for (int m = 0; m < _desc.plugin->modules.size(); m++)
  {
    auto const& module = _desc.plugin->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        for(int pi = 0; pi < param.slot_count; pi++)
          if(param.rate == param_rate::accurate)
            for(int f = 0; f < _common_block.frame_count; f++)
              _plugin_block.in.accurate[m][mi][p][pi][f] = param.normalized_to_plain(
                normalized_value(_plugin_block.in.accurate[m][mi][p][pi][f])).real();
      }
  }

  // run all modules in order
  _plugin_block.sample_rate = _sample_rate;
  for(int m = 0; m < _desc.plugin->modules.size(); m++)
    for(int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
    {
      module_block block;
      block.out.params_ = &_state[m][mi];
      block.out.cv_ = &_plugin_block.out.cv[m][mi];
      block.out.audio_ = &_plugin_block.out.audio[m][mi];
      block.in.block_ = &_plugin_block.in.block[m][mi];
      block.in.accurate_ = &_plugin_block.in.accurate[m][mi];
      _module_engines[m][mi]->process(*_desc.plugin, _plugin_block, block);
    }

  // update output params 3 times a second
  _host_block.events.out.clear();
  auto now_ticks = std::chrono::system_clock::now().time_since_epoch();
  auto now_millis = std::chrono::duration_cast<std::chrono::milliseconds>(now_ticks);
  if((now_millis - _activated_at_ms).count() < 333) return;

  // push all out params - we don't check for changes
  int param_global = 0;
  _activated_at_ms = now_millis;
  for (int m = 0; m < _desc.plugin->modules.size(); m++)
  {
    auto const& module = _desc.plugin->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for(int pi = 0; pi < module.params[p].slot_count; pi++)
        {
          if (module.params[p].dir == param_dir::output)
          {
            block_event out_event;
            out_event.param = param_global;
            out_event.normalized = module.params[p].plain_to_normalized(_state[m][mi][p][pi]);
            _host_block.events.out.push_back(out_event);
          }
          param_global++;
        }
  }
}

}