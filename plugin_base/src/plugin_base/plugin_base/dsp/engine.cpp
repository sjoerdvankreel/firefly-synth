#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/block/host.hpp>
#include <plugin_base/desc/frame_dims.hpp>

#include <limits>
#include <algorithm>

namespace plugin_base {

plugin_engine::
plugin_engine(
  plugin_desc const* desc,
  thread_pool_voice_processor voice_processor,
  void* voice_processor_context) :
_state(desc, false), _block_automation(desc, false),
_dims(*desc->plugin),
_host_block(std::make_unique<host_block>()),
_voice_processor(voice_processor),
_voice_thread_ids(_state.desc().plugin->polyphony, std::thread::id()),
_voice_processor_context(voice_processor_context)
{
  // validate here instead of plugin_desc ctor 
  // since that runs on module init so is hard to debug
  desc->validate();

  // reserve this much but allocate on the audio thread if necessary
  // still seems better than dropping events
  int block_events_guess = _state.desc().param_count;
  int midi_events_guess = _state.desc().midi_count * 64;
  int accurate_events_guess = _state.desc().param_count * 64;
  int note_limit_guess = _state.desc().plugin->polyphony * 64;

  // init everything that is not frame-count dependent
  _global_context.resize(_dims.module_slot);
  _voice_context.resize(_dims.voice_module_slot);
  _output_values.resize(_dims.module_slot_param_slot);
  _input_engines.resize(_dims.module_slot);
  _output_engines.resize(_dims.module_slot);
  _voice_engines.resize(_dims.voice_module_slot);
  _midi_frames.resize(_state.desc().midi_count);
  _midi_was_automated.resize(_state.desc().midi_count);
  _accurate_frames.resize(_state.desc().param_count);
  _voice_states.resize(_state.desc().plugin->polyphony);
  _host_block->events.notes.reserve(note_limit_guess);
  _host_block->events.out.reserve(block_events_guess);
  _host_block->events.block.reserve(block_events_guess);
  _host_block->events.midi.reserve(midi_events_guess);
  _host_block->events.accurate.reserve(accurate_events_guess);

  _midi_values.resize(_dims.module_slot_midi);
  for(int m = 0; m < desc->plugin->modules.size(); m++)
    for(int mi = 0; mi < desc->plugin->modules[m].info.slot_count; mi++)
      for(int ms = 0; ms < desc->plugin->modules[m].midi_sources.size(); ms++)
        _midi_values[m][mi][ms] = desc->plugin->modules[m].midi_sources[ms].default_;
}

plugin_block
plugin_engine::make_plugin_block(
  int voice, int module, int slot, 
  int start_frame, int end_frame)
{
  void** context_out = voice < 0
    ? &_global_context[module][slot]
    : &_voice_context[voice][module][slot];
  jarray<float, 3>& cv_out = voice < 0
    ? _global_cv_state[module][slot]
    : _voice_cv_state[voice][module][slot];
  jarray<float, 2>& scratch = voice < 0
    ? _global_scratch_state[module][slot]
    : _voice_scratch_state[voice][module][slot];
  jarray<float, 4>& audio_out = voice < 0
    ? _global_audio_state[module][slot] 
    : _voice_audio_state[voice][module][slot];
  plugin_block_state state = {
    context_out, cv_out, audio_out, scratch,
    _global_cv_state, _global_audio_state, _global_context, _global_scratch_state, 
    _midi_automation[module][slot], _midi_automation,
    _accurate_automation[module][slot], _accurate_automation,
    _block_automation.state()[module][slot], _block_automation.state()
  };
  return {
    start_frame, end_frame, slot,
    _sample_rate, state, nullptr, nullptr, 
    _host_block->shared, *_state.desc().plugin, 
    _state.desc().plugin->modules[module]
  };
}

void
plugin_engine::release_block()
{
  double now = seconds_since_epoch();
  double process_time_sec = now - _block_start_time_sec;
  double block_time_sec = _host_block->frame_count / _sample_rate;
  _cpu_usage = process_time_sec / block_time_sec;
}

host_block&
plugin_engine::prepare_block()
{
  // host calls this and should provide the current block values
  _block_start_time_sec = seconds_since_epoch();
  _host_block->audio_out = nullptr;
  _host_block->events.out.clear();
  _host_block->events.notes.clear();
  _host_block->events.block.clear();
  _host_block->events.midi.clear();
  _host_block->events.accurate.clear();
  _host_block->frame_count = 0;
  _host_block->shared.bpm = 0;
  _host_block->shared.audio_in = nullptr;
  return *_host_block;
}

void
plugin_engine::deactivate()
{
  _cpu_usage = 0;
  _sample_rate = 0;
  _stream_time = 0;
  _output_updated_sec = 0;
  _block_start_time_sec = 0;

  // drop frame-count dependent memory
  _voice_results = {};
  _voices_mixdown = {};
  _voice_cv_state = {};
  _voice_audio_state = {};
  _global_cv_state = {};
  _global_audio_state = {};
  _midi_automation = {};
  _accurate_automation = {};
  _voice_scratch_state = {};
  _global_scratch_state = {};
  _host_block->events.out.clear();
  _host_block->events.midi.clear();
  _host_block->events.block.clear();
  _host_block->events.accurate.clear();

  for(int m = 0; m < _state.desc().module_voice_start; m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
      _input_engines[m][mi].reset();
  for (int m = _state.desc().module_voice_start; m < _state.desc().module_output_start; m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
      for (int v = 0; v < _state.desc().plugin->polyphony; v++)
        _voice_engines[v][m][mi].reset();
  for (int m = _state.desc().module_output_start; m < _state.desc().plugin->modules.size(); m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
      _output_engines[m][mi].reset();
}

void
plugin_engine::activate(int sample_rate, int max_frame_count)
{  
  deactivate();
  _stream_time = 0;
  _sample_rate = sample_rate;
  _output_updated_sec = seconds_since_epoch();

  // init frame-count dependent memory
  plugin_frame_dims frame_dims(*_state.desc().plugin, max_frame_count);
  _voices_mixdown.resize(frame_dims.audio);
  _voice_results.resize(frame_dims.voices_audio);
  _voice_cv_state.resize(frame_dims.module_voice_cv);
  _voice_scratch_state.resize(frame_dims.module_voice_scratch);
  _voice_audio_state.resize(frame_dims.module_voice_audio);
  _global_cv_state.resize(frame_dims.module_global_cv);
  _global_scratch_state.resize(frame_dims.module_global_scratch);
  _global_audio_state.resize(frame_dims.module_global_audio);
  _midi_automation.resize(frame_dims.midi_automation);
  _accurate_automation.resize(frame_dims.accurate_automation);

  for (int m = 0; m < _state.desc().module_voice_start; m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
      _input_engines[m][mi] = _state.desc().plugin->modules[m].engine_factory(*_state.desc().plugin, sample_rate, max_frame_count);
  for (int m = _state.desc().module_voice_start; m < _state.desc().module_output_start; m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
      for (int v = 0; v < _state.desc().plugin->polyphony; v++)
        _voice_engines[v][m][mi] = _state.desc().plugin->modules[m].engine_factory(*_state.desc().plugin, sample_rate, max_frame_count);
  for (int m = _state.desc().module_output_start; m < _state.desc().plugin->modules.size(); m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
      _output_engines[m][mi] = _state.desc().plugin->modules[m].engine_factory(*_state.desc().plugin, sample_rate, max_frame_count);
}

void 
plugin_engine::process_voices_single_threaded()
{
  for (int v = 0; v < _voice_states.size(); v++)
    if (_voice_states[v].stage != voice_stage::unused)
      process_voice(v, false);
}

void
plugin_engine::process_voice(int v, bool threaded)
{
  // simplifies threadpool
  // so we can just push (polyphony) tasks each time
  if (_voice_states[v].stage == voice_stage::unused) return;

  auto& state = _voice_states[v];
  std::pair<std::uint32_t, std::uint32_t> denormal_state;
  if(threaded) denormal_state = disable_denormals();
  for (int m = _state.desc().module_voice_start; m < _state.desc().module_output_start; m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
    {
      plugin_voice_block voice_block = {
        false, _voice_results[v], state,
        _voice_cv_state[v], _voice_audio_state[v], _voice_context[v], _voice_scratch_state[v]
      };
      plugin_block block(make_plugin_block(v, m, mi, state.start_frame, state.end_frame));
      block.voice = &voice_block;
      _voice_engines[v][m][mi]->process(block);

      // plugin completed its envelope
      if (block.voice->finished)
        _voice_states[v].stage = voice_stage::finishing;
    }

  // plugin should have initiated release state
  state.release_frame = -1;

  // process() call will acquire
  if (threaded)
  {
    _voice_thread_ids[v] = std::this_thread::get_id();
    std::atomic_thread_fence(std::memory_order_release);
    restore_denormals(denormal_state);
  }
}

void 
plugin_engine::process()
{
  int voice_count = 0;
  int frame_count = _host_block->frame_count;
  std::pair<std::uint32_t, std::uint32_t> denormal_state = disable_denormals();
  scope_guard denormals([denormal_state]() { restore_denormals(denormal_state); });

  // TODO monophonic portamento

  // always take a voice for an entire block,
  // module processor is handed appropriate start/end_frame.
  // and return voices completed the previous block
  for (int i = 0; i < _voice_states.size(); i++)
  {
    auto& state = _voice_states[i];
    if (state.stage == voice_stage::active || state.stage == voice_stage::releasing)
    {
      voice_count++;
      state.start_frame = 0;
      state.end_frame = frame_count;
      state.release_frame = frame_count;
    }
    else if (state.stage == voice_stage::finishing)
      state = voice_state();
  }

  // steal voices for incoming notes by age
  for (int e = 0; e < _host_block->events.notes.size(); e++)
  {
    int slot = -1;
    auto const& event = _host_block->events.notes[e];
    if (event.type != note_event_type::on) continue;
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

    for (int m = _state.desc().module_voice_start; m < _state.desc().module_output_start; m++)
      for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
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
    if (event.type == note_event_type::on) continue;
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
        if(event.type == note_event_type::cut)
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
  for(int m = 0; m < _state.desc().plugin->modules.size(); m++)
  {
    auto const& module = _state.desc().plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      if(module.dsp.output == module_output::cv)
        if(module.dsp.stage != module_stage::voice)
        {
          for(int o = 0; o < module.dsp.outputs.size(); o++)
            for(int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
              _global_cv_state[m][mi][o][oi].fill(0, frame_count, 0.0f);
        } else {
          for (int v = 0; v < _voice_states.size(); v++)
            if(_voice_states[v].stage != voice_stage::unused)
              for (int o = 0; o < module.dsp.outputs.size(); o++)
                for (int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
                  _voice_cv_state[v][m][mi][o][oi].fill(0, frame_count, 0.0f);
        }
      else if (module.dsp.output == module_output::audio)
        if (module.dsp.stage != module_stage::voice)
        {
          for (int o = 0; o < module.dsp.outputs.size(); o++)
            for (int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
              for(int c = 0; c < 2; c++)
                _global_audio_state[m][mi][o][oi][c].fill(0, frame_count, 0.0f);
        } else {
           for (int v = 0; v < _voice_states.size(); v++)
             if (_voice_states[v].stage != voice_stage::unused)
               for (int o = 0; o < module.dsp.outputs.size(); o++)
                 for (int oi = 0; oi < module.dsp.outputs[o].info.slot_count; oi++)
                   for (int c = 0; c < 2; c++)
                     _voice_audio_state[v][m][mi][o][oi][c].fill(0, frame_count, 0.0f);
        }
      else
        assert(module.dsp.output == module_output::none);
  }

  // set automation values to state, automation may overwrite
  // accurate goes from plain to normalized as interpolation must be done in normalized
  for (int m = 0; m < _state.desc().plugin->modules.size(); m++)
  {
    auto const& module = _state.desc().plugin->modules[m];
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      for (int ms = 0; ms < module.midi_sources.size(); ms++)
        std::fill(
          _midi_automation[m][mi][ms].begin(),
          _midi_automation[m][mi][ms].begin() + frame_count,
          _midi_values[m][mi][ms]);

      for(int p = 0; p < module.params.size(); p++)
      {
        auto const& param = module.params[p];
        if(param.dsp.rate == param_rate::block)
          for(int pi = 0; pi < param.info.slot_count; pi++)
            _block_automation.set_plain_at(m, mi, p, pi, _state.get_plain_at(m, mi, p, pi));
        else
          for (int pi = 0; pi < param.info.slot_count; pi++)
            std::fill(
              _accurate_automation[m][mi][p][pi].begin(),
              _accurate_automation[m][mi][p][pi].begin() + frame_count,
              (float)_state.get_normalized_at(m, mi, p, pi).value());
      }
    }
  }
    
  // process per-block automation values
  for (int e = 0; e < _host_block->events.block.size(); e++)
  {
    auto const& event = _host_block->events.block[e];
    _state.set_normalized_at_index(event.param, event.normalized);
    _block_automation.set_normalized_at_index(event.param, event.normalized);
  }

  // process accurate automation values, this is a bit tricky as 
  // 1) events can come in at any frame position and
  // 2) interpolation needs to be done on normalized (linear) values (plugin needs to scale to plain) and
  // 3) the host must provide a value at the end of the buffer if that event would have effect w.r.t. the next block
  std::fill(_accurate_frames.begin(), _accurate_frames.end(), 0);
  for (int e = 0; e < _host_block->events.accurate.size(); e++)
  {
    // linear interpolate as normalized
    auto const& event = _host_block->events.accurate[e];
    auto const& mapping = _state.desc().param_mappings.params[event.param];
    auto& curve = mapping.topo.value_at(_accurate_automation);
    int prev_frame = _accurate_frames[event.param];
    float range_frames = event.frame - prev_frame + 1;
    float range = event.normalized.value() - curve[prev_frame];
    for(int f = prev_frame; f <= event.frame; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / range_frames * range;

    // update current state
    _accurate_frames[event.param] = event.frame;
    _state.set_normalized_at_index(event.param, event.normalized);
  }

  // process midi automation values
  // pretty much the same as regular (accurate) parameter automation, but see below
  std::fill(_midi_frames.begin(), _midi_frames.end(), 0);
  std::fill(_midi_was_automated.begin(), _midi_was_automated.end(), 0);
  for (int e = 0; e < _host_block->events.midi.size(); e++)
  {
    // linear interpolate
    auto const& event = _host_block->events.midi[e];
    auto const& id_mapping = _state.desc().midi_mappings.id_to_index;
    auto iter = id_mapping.find(event.id);
    if(iter == id_mapping.end()) continue;
    int midi_index = iter->second;
    _midi_was_automated[midi_index] = 1;
    auto const& mapping = _state.desc().midi_mappings.midi_sources[midi_index];
    auto& curve = mapping.topo.value_at(_midi_automation);
    int prev_frame = _midi_frames[midi_index];
    float range_frames = event.frame - prev_frame + 1;
    float range = event.normalized.value() - curve[prev_frame];
    for (int f = prev_frame; f <= event.frame; f++)
      curve[f] = curve[prev_frame] + (f - prev_frame) / range_frames * range;

    // update current state
    _midi_frames[midi_index] = event.frame;
    _midi_values[mapping.topo.module_index][mapping.topo.module_slot][mapping.topo.midi_index] = event.normalized.value();
  }

  // but: it seems not all hosts transmit midi events at block boundaries
  // like it's done for parameter automation (not even for built-in host midi automation)
  // so we need to take that into account. best we can do is just re-use the last midi value
  for(int i = 0; i < _midi_was_automated.size(); i++)
    if (_midi_was_automated[i])
    {
      auto const& mapping = _state.desc().midi_mappings.midi_sources[i];
      auto& curve = mapping.topo.value_at(_midi_automation);
      for(int f = _midi_frames[i]; f < frame_count; f++)
        curve[f] = curve[_midi_frames[i]];
    }

  // run input modules in order
  for (int m = 0; m < _state.desc().module_voice_start; m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
    {
      plugin_block block(make_plugin_block(-1, m, mi, 0, frame_count));
      _input_engines[m][mi]->process(block);
    }

  // run voice modules in order taking advantage of host threadpool if possible
  // note: multithreading over voices, not anything within a single voice
  int thread_count = 1;
  if (!_voice_processor)
    process_voices_single_threaded();
  else
  {
    for (int v = 0; v < _state.desc().plugin->polyphony; v++)
      _voice_thread_ids[v] = std::thread::id();
    std::atomic_thread_fence(std::memory_order_release);
    if (!_voice_processor(*this, _voice_processor_context))
      process_voices_single_threaded();
    else 
    {
      std::atomic_thread_fence(std::memory_order_acquire);
      std::sort(_voice_thread_ids.begin(), _voice_thread_ids.end());
      thread_count = std::unique(_voice_thread_ids.begin(), _voice_thread_ids.end()) - _voice_thread_ids.begin();
      thread_count = std::max(1, thread_count);
    }
  }

  // combine voices output
  for (int v = 0; v < _voice_states.size(); v++)
    if (_voice_states[v].stage != voice_stage::unused)
      for(int c = 0; c < 2; c++)
        for(int f = 0; f < frame_count; f++)
          _voices_mixdown[c][f] += _voice_results[v][c][f];

  // run output modules in order
  for (int m = _state.desc().module_output_start; m < _state.desc().plugin->modules.size(); m++)
    for (int mi = 0; mi < _state.desc().plugin->modules[m].info.slot_count; mi++)
    {
      // output params are written to intermediate buffer first
      plugin_output_block out_block = {
        voice_count, thread_count,
        _cpu_usage, _host_block->audio_out,
        _output_values[m][mi], _voices_mixdown
      };
      plugin_block block(make_plugin_block(-1, m, mi, 0, frame_count));
      block.out = &out_block;
      _output_engines[m][mi]->process(block);

      // copy back output parameter values
      for(int p = 0; p < _state.desc().plugin->modules[m].params.size(); p++)
        if(_state.desc().plugin->modules[m].params[p].dsp.direction == param_direction::output)
          for(int pi = 0; pi < _state.desc().plugin->modules[m].params[p].info.slot_count; pi++)
            _state.set_plain_at(m, mi, p, pi, _output_values[m][mi][p][pi]);
    }

  // keep track of running time in frames
  _stream_time += frame_count;

  // update output params 3 times a second
  // push all out params - we don't check for changes
  _host_block->events.out.clear();
  auto now_sec = seconds_since_epoch();
  if(now_sec - _output_updated_sec > 0.33)
  {
    int param_global = 0;
    _output_updated_sec = now_sec;
    for (int m = 0; m < _state.desc().plugin->modules.size(); m++)
    {
      auto const& module = _state.desc().plugin->modules[m];
      for (int mi = 0; mi < module.info.slot_count; mi++)
        for (int p = 0; p < module.params.size(); p++)
          for(int pi = 0; pi < module.params[p].info.slot_count; pi++)
          {
            if (module.params[p].dsp.direction == param_direction::output)
            {
              block_event out_event;
              out_event.param = param_global;
              out_event.normalized = _state.get_normalized_at(m, mi, p, pi);
              _host_block->events.out.push_back(out_event);
            }
            param_global++;
          }
    }
  }
}

}