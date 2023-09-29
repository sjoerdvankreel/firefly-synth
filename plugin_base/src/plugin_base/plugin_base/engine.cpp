#include <plugin_base/engine.hpp>
#include <plugin_base/block_host.hpp>

#include <limits>

namespace plugin_base {

plugin_engine::
plugin_engine(std::unique_ptr<plugin_topo>&& topo) :
_desc(std::move(topo)), _dims(*_desc.plugin), 
_host_block(std::make_unique<host_block>())
{
  // reserve this much but allocate on the audio thread if necessary
  // still seems better than dropping events
  int block_events_guess = _desc.param_count;
  int accurate_events_guess = _desc.param_count * 64;
  int note_limit_guess = _desc.plugin->polyphony * 64;

  // init everything that is not frame-count dependent
  _state.resize(_dims.module_slot_param_slot);
  _desc.init_defaults(_state);
  _input_engines.resize(_dims.module_slot);
  _output_engines.resize(_dims.module_slot);
  _voice_engines.resize(_dims.voice_module_slot);
  _accurate_frames.resize(_desc.param_count);
  _voice_states.resize(_desc.plugin->polyphony);
  _block_automation.resize(_dims.module_slot_param_slot);
  _host_block->events.notes.reserve(note_limit_guess);
  _host_block->events.out.reserve(block_events_guess);
  _host_block->events.block.reserve(block_events_guess);
  _host_block->events.accurate.reserve(accurate_events_guess);
}

process_block 
plugin_engine::make_process_block(int voice, int module, int slot, int start_frame, int end_frame)
{
  jarray<float, 1>& cv_out = voice < 0? _global_cv_state[module][slot]: _voice_cv_state[voice][module][slot];
  jarray<float, 2>& audio_out = voice < 0 ? _global_audio_state[module][slot] : _voice_audio_state[voice][module][slot];
  return {
    start_frame, end_frame,
    _sample_rate, nullptr, _host_block->common,
    *_desc.plugin, _desc.plugin->modules[module], nullptr,
    cv_out, audio_out, _global_cv_state, _global_audio_state,
    _accurate_automation[module][slot], _block_automation[module][slot]
  };
}

host_block&
plugin_engine::prepare()
{
  // host calls this and should provide the current block values
  _host_block->audio_out = nullptr;
  _host_block->events.out.clear();
  _host_block->events.notes.clear();
  _host_block->events.block.clear();
  _host_block->events.accurate.clear();
  _host_block->frame_count = 0;
  _host_block->common.bpm = 0;
  _host_block->common.audio_in = nullptr;
  return *_host_block;
}

void
plugin_engine::deactivate()
{
  _sample_rate = 0;
  _stream_time = 0;
  _output_updated_ms = {};

  // drop frame-count dependent memory
  _voice_results = {};
  _voices_mixdown = {};
  _voice_cv_state = {};
  _voice_audio_state = {};
  _global_cv_state = {};
  _global_audio_state = {};
  _accurate_automation = {};
  _host_block->events.out.clear();
  _host_block->events.block.clear();
  _host_block->events.accurate.clear();

  for(int m = 0; m < _desc.module_voice_start; m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      _input_engines[m][mi].reset();
  for (int m = _desc.module_voice_start; m < _desc.module_output_start; m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      for (int v = 0; v < _desc.plugin->polyphony; v++)
        _voice_engines[v][m][mi].reset();
  for (int m = _desc.module_output_start; m < _desc.plugin->modules.size(); m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      _output_engines[m][mi].reset();
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{  
  deactivate();
  _stream_time = 0;
  _sample_rate = sample_rate;

  // set activation time
  auto now_ticks = std::chrono::system_clock::now().time_since_epoch();
  _output_updated_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now_ticks);

  // init frame-count dependent memory
  plugin_frame_dims frame_dims(*_desc.plugin, max_frame_count);
  _voices_mixdown.resize(frame_dims.audio);
  _voice_results.resize(frame_dims.voices_audio);
  _voice_cv_state.resize(frame_dims.module_voice_cv);
  _voice_audio_state.resize(frame_dims.module_voice_audio);
  _global_cv_state.resize(frame_dims.module_global_cv);
  _global_audio_state.resize(frame_dims.module_global_audio);
  _accurate_automation.resize(frame_dims.accurate_automation);

  for (int m = 0; m < _desc.module_voice_start; m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      _input_engines[m][mi] = _desc.plugin->modules[m].engine_factory(sample_rate, max_frame_count);
  for (int m = _desc.module_voice_start; m < _desc.module_output_start; m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      for (int v = 0; v < _desc.plugin->polyphony; v++)
        _voice_engines[v][m][mi] = _desc.plugin->modules[m].engine_factory(sample_rate, max_frame_count);
  for (int m = _desc.module_output_start; m < _desc.plugin->modules.size(); m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
      _output_engines[m][mi] = _desc.plugin->modules[m].engine_factory(sample_rate, max_frame_count);
}

void 
plugin_engine::process()
{
  int frame_count = _host_block->frame_count;

  // TODO monophonic portamento

  // always take a voice for an entire block,
  // module processor is handed appropriate start/end_frame.
  // and return voices completed the previous block
  for(int i = 0; i < _voice_states.size(); i++)
    if (_voice_states[i].stage == voice_stage::active)
    {
      _voice_states[i].start_frame = 0;
      _voice_states[i].end_frame = frame_count;
    } else if(_voice_states[i].stage == voice_stage::finishing)
      _voice_states[i] = voice_state();

  // steal voices for incoming notes by age
  for (int e = 0; e < _host_block->events.notes.size(); e++)
  {
    int slot = -1;
    auto const& event = _host_block->events.notes[e];
    if (event.type != note_event::type_t::on) continue;
    std::int64_t min_time = std::numeric_limits<std::int64_t>::max();
    for (int i = 0; i < _voice_states.size(); i++)
      if (_voice_states[i].stage == voice_stage::unused)
      {
        slot = i;
        break;
      } else if(_voice_states[i].time < min_time)
      {
        slot = i;
        min_time = _voice_states[i].time;
      }
    assert(slot >= 0); 
    auto& state = _voice_states[slot];
    state.id = event.id;
    state.end_frame = frame_count;
    state.start_frame = event.frame;
    state.velocity = event.velocity;
    state.stage = voice_stage::active;
    state.time = _stream_time + event.frame;
    assert(0 <= state.start_frame && state.start_frame <= state.end_frame && state.end_frame <= frame_count);

    for (int m = _desc.module_voice_start; m < _desc.module_output_start; m++)
      for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
        _voice_engines[slot][m][mi]->initialize();
  }
  
  // mark voices for completion the next block
  // be sure to check the note was on (earlier in time than the event) before we turn it off
  // not sure how to handle note id vs pck: 
  // clap on bitwig hands us note-on events with note id and note-off events without them
  // so i choose to allow note-off with note-id to only kill the same note-id
  // but allow note-off without note-id to kill any matching pck regardless of note-id
  for (int e = 0; e < _host_block->events.notes.size(); e++)
  {
    auto const& event = _host_block->events.notes[e];
    if (event.type == note_event::type_t::on) continue;
    int release_count = 0;
    (void)release_count;
    for (int v = 0; v < _voice_states.size(); v++)
    {
      auto& state = _voice_states[v];
      if (state.stage == voice_stage::active &&
        state.time < _stream_time + event.frame &&
        ((event.id.id != -1 && state.id.id == event.id.id) ||
        (event.id.id == -1 && (state.id.key == event.id.key && state.id.channel == event.id.channel))))
      {
        release_count++;
        if(event.type == note_event::type_t::cut)
        {
          state.end_frame = event.frame;
          state.stage = voice_stage::finishing;
          assert(0 <= state.start_frame && state.start_frame <= state.end_frame && state.end_frame < frame_count);
        } else {
          state.release_frame = event.frame;
          state.stage = voice_stage::releasing;
          assert(0 <= state.start_frame && state.start_frame <= state.release_frame && state.release_frame < frame_count);
        }
      }
    }
    assert(release_count > 0);
  }

  // clear audio outputs
  for (int c = 0; c < 2; c++)
  {
    std::fill(_host_block->audio_out[c], _host_block->audio_out[c] + frame_count, 0.0f);
    std::fill(_voices_mixdown[c].begin(), _voices_mixdown[c].begin() + frame_count, 0.0f);
    for (int v = 0; v < _voice_results.size(); v++)
      if(_voice_states[v].stage != voice_stage::unused)
        std::fill(_voice_results[v][c].begin(), _voice_results[v][c].begin() + frame_count, 0.0f);
  }

  // clear module cv/audio out
  for(int m = 0; m < _desc.plugin->modules.size(); m++)
  {
    auto const& module = _desc.plugin->modules[m];
    for (int mi = 0; mi < module.slot_count; mi++)
      if(module.output == module_output::cv)
        if(module.stage != module_stage::voice)
        {
          std::fill(_global_cv_state[m][mi].begin(), 
                    _global_cv_state[m][mi].begin() + frame_count, 0.0f);
        } else {
          for (int v = 0; v < _voice_states.size(); v++)
            if(_voice_states[v].stage != voice_stage::unused)
              std::fill(_voice_cv_state[v][m][mi].begin(), 
                        _voice_cv_state[v][m][mi].begin() + frame_count, 0.0f);
        }
      else if (module.output == module_output::audio)
        if (module.stage != module_stage::voice)
        {
          for(int c = 0; c < 2; c++)
            std::fill(_global_audio_state[m][mi][c].begin(), 
                     _global_audio_state[m][mi][c].begin() + frame_count, 0.0f);
        } else {
           for (int v = 0; v < _voice_states.size(); v++)
             if (_voice_states[v].stage != voice_stage::unused)
               for (int c = 0; c < 2; c++)
                 std::fill(_voice_audio_state[v][m][mi][c].begin(), 
                           _voice_audio_state[v][m][mi][c].begin() + frame_count, 0.0f);
        }
      else
        assert(module.output == module_output::none);
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
            _block_automation[m][mi][p][pi] = _state[m][mi][p][pi];
        else
          for (int pi = 0; pi < param.slot_count; pi++)
            std::fill(
              _accurate_automation[m][mi][p][pi].begin(),
              _accurate_automation[m][mi][p][pi].begin() + frame_count,
              (float)param.raw_to_normalized(_state[m][mi][p][pi].real()).value());
      }
  }
    
  // process per-block automation values
  for (int e = 0; e < _host_block->events.block.size(); e++)
  {
    auto const& event = _host_block->events.block[e];
    auto const& mapping = _desc.mappings[event.param];
    plain_value plain = _desc.param_at(mapping).param->normalized_to_plain(event.normalized);
    mapping.value_at(_state) = plain;
    mapping.value_at(_block_automation) = plain;
  }

  // process accurate automation values, this is a bit tricky as 
  // 1) events can come in at any frame position and
  // 2) interpolation needs to be done on normalized (linear) values and
  // 3) the host must provide a value at the end of the buffer if that event would have effect w.r.t. the next block
  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block->events.accurate.size(); e++)
  {
    // linear interpolate as normalized
    auto const& event = _host_block->events.accurate[e];
    auto const& mapping = _desc.mappings[event.param];
    auto& curve = mapping.value_at(_accurate_automation);
    int prev_frame = _accurate_frames[event.param];
    float range_frames = event.frame - prev_frame + 1;
    float range = event.normalized.value() - curve[prev_frame];
    for(int f = prev_frame; f <= event.frame; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / range_frames * range;

    // denormalize
    mapping.value_at(_state).real_unchecked(_desc.param_at(mapping).param->normalized_to_plain(event.normalized).real());
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
            for(int f = 0; f < frame_count; f++)
              _accurate_automation[m][mi][p][pi][f] = param.normalized_to_plain(
                normalized_value(_accurate_automation[m][mi][p][pi][f])).real();
      }
  }

  // run input modules in order
  for (int m = 0; m < _desc.module_voice_start; m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
    {
      process_block block(make_process_block(-1, m, mi, 0, frame_count));
      _input_engines[m][mi]->process(block);
    }

  // run voice modules in order
  // TODO threadpool
  for (int v = 0; v < _voice_states.size(); v++)
    if (_voice_states[v].stage != voice_stage::unused)
      for (int m = _desc.module_voice_start; m < _desc.module_output_start; m++)
        for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
        {
          auto& state = _voice_states[v];
          voice_process_block voice_block = {
            false,
            _voice_results[v],
            state,
            _voice_cv_state[v],
            _voice_audio_state[v]
          };
          process_block block(make_process_block(v, m, mi, state.start_frame, state.end_frame));
          block.voice = &voice_block;
          _voice_engines[v][m][mi]->process(block);
          state.release_frame = -1;

          // plugin completed its envelope
          if(block.voice->finished) 
            _voice_states[v].stage = voice_stage::finishing;
        }

  // combine voices output
  for (int v = 0; v < _voice_states.size(); v++)
    if (_voice_states[v].stage != voice_stage::unused)
      for(int c = 0; c < 2; c++)
        for(int f = 0; f < frame_count; f++)
          _voices_mixdown[c][f] += _voice_results[v][c][f];

  // combine output modules in order
  for (int m = _desc.module_output_start; m < _desc.plugin->modules.size(); m++)
    for (int mi = 0; mi < _desc.plugin->modules[m].slot_count; mi++)
    {
      out_process_block out_block = {
        _host_block->audio_out,
        _state[m][mi],
        _voices_mixdown
      };
      process_block block(make_process_block(-1, m, mi, 0, frame_count));
      block.out = &out_block;
      _output_engines[m][mi]->process(block);
    }

  // keep track of running time in frames
  _stream_time += frame_count;

  // update output params 3 times a second
  // push all out params - we don't check for changes
  _host_block->events.out.clear();
  auto now_ticks = std::chrono::system_clock::now().time_since_epoch();
  auto now_millis = std::chrono::duration_cast<std::chrono::milliseconds>(now_ticks);
  if((now_millis - _output_updated_ms).count() >= 333)
  {
    int param_global = 0;
    _output_updated_ms = now_millis;
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
              _host_block->events.out.push_back(out_event);
            }
            param_global++;
          }
    }
  }
}

}