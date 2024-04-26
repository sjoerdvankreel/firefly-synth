#include <plugin_base/dsp/splice_engine.hpp>

namespace plugin_base {
  
plugin_splice_engine::
plugin_splice_engine(
  plugin_desc const* desc, bool graph,
  thread_pool_voice_processor voice_processor,
  void* voice_processor_context):
_engine(desc, graph, voice_processor, voice_processor_context) {}

host_block&
plugin_splice_engine::prepare_block()
{
  _host_block.prepare();
  return _host_block;
}

void
plugin_splice_engine::deactivate()
{
  _engine.deactivate();
  _host_block.events.deactivate();
  _splice_block_size = -1;
}

void 
plugin_splice_engine::activate(int max_frame_count)
{
  // WASAPI likes multiples of 32 (160 samples = 3ms @ 48khz).
  // Other stuff likes powers of 2. 
  // Below 128 samples performance impact gets noticeable. So:
  // 1) if host size < 128, go with host size
  // 2) if host size is multiple of 128, go with 128
  // 3) if host size is multiple of 160, go with 160
  // 4) else just go with 128 and hope for the best

  if(max_frame_count < 128) _splice_block_size = max_frame_count;
  else if(max_frame_count % 128 == 0) _splice_block_size = 128;
  else if(max_frame_count % 160 == 0) _splice_block_size = 160;
  else _splice_block_size = 128;

  _engine.activate(_splice_block_size);
  _host_block.events.activate(false, state().desc().param_count, state().desc().midi_count, state().desc().plugin->audio_polyphony, max_frame_count);
}

void
plugin_splice_engine::process()
{
  // process in blocks of splice_size, plus 1 times remaining
  int spliced_block_frames = _splice_block_size;
  int spliced_block_count = _host_block.frame_count / spliced_block_frames;
  int rest_block_frames = _host_block.frame_count - spliced_block_count * spliced_block_frames;
  assert(rest_block_frames < spliced_block_frames);
  assert(spliced_block_count * spliced_block_frames + rest_block_frames == _host_block.frame_count);
  int total_block_count = spliced_block_count + (rest_block_frames == 0? 0: 1);

  _host_block.events.out.clear();
  auto comp = [](auto const& l, auto const& r) { return l.param < r.param ? true : l.param > r.param ? false : l.frame < r.frame; };
  std::sort(_host_block.events.accurate.begin(), _host_block.events.accurate.end(), comp);

  for (int i = 0; i < total_block_count; i++)
  {
    int this_block_start = i * spliced_block_frames;
    int this_block_frames = i < spliced_block_count? spliced_block_frames: rest_block_frames;

    auto& inner_block = _engine.prepare_block();
    inner_block.frame_count = this_block_frames;
    inner_block.shared.bpm = _host_block.shared.bpm;

    float* this_audio_out[2];
    this_audio_out[0] = _host_block.audio_out[0] + this_block_start;
    this_audio_out[1] = _host_block.audio_out[1] + this_block_start;
    inner_block.audio_out = this_audio_out;

    float const* this_audio_in[2] = { nullptr, nullptr };
    if(this->state().desc().plugin->type == plugin_type::fx)
    {
      this_audio_in[0] = _host_block.shared.audio_in[0] + this_block_start;
      this_audio_in[1] = _host_block.shared.audio_in[1] + this_block_start;
      inner_block.shared.audio_in = this_audio_in;
    }

    inner_block.events.midi.clear();
    inner_block.events.block.clear();
    inner_block.events.notes.clear();
    inner_block.events.accurate.clear();

    // this could be made more efficient but i doubt if it would matter

    // for block events, just repeat on each start
    for (int b = 0; b < _host_block.events.block.size(); b++)
      inner_block.events.block.push_back(_host_block.events.block[b]);

    // note events are easy, just adjust for block start
    for (int n = 0; n < _host_block.events.notes.size(); n++)
      if (_host_block.events.notes[n].frame >= this_block_start && _host_block.events.notes[n].frame < this_block_start + this_block_frames)
      {
        note_event e = _host_block.events.notes[n];
        e.frame -= this_block_start;
        inner_block.events.notes.push_back(e);
      }

    // midi events are easy too, also just adjust for block start
    // plugin_engine will smooth jumps using lpf optionally controlled by the plugin
    for(int m = 0; m < _host_block.events.midi.size(); m++)
      if(_host_block.events.midi[m].frame >= this_block_start && _host_block.events.midi[m].frame < this_block_start + this_block_frames)
      {
        midi_event e = _host_block.events.midi[m];
        e.frame -= this_block_start;
        inner_block.events.midi.push_back(e);
      }

    // accurate is some bookkeeping
    // host transmits minimum set of events that allows to reconstruct the curve
    // see https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html#problems
    // plugin_engine assumes exactly this format, so we have to accomodate that while splitting
    
    // copy over explicit events
    for (int a = 0; a < _host_block.events.accurate.size(); a++)
      if (_host_block.events.accurate[a].frame >= this_block_start && _host_block.events.accurate[a].frame < this_block_start + this_block_frames)
      {
        accurate_event e = _host_block.events.accurate[a];
        e.frame -= this_block_start;
        inner_block.events.accurate.push_back(e);
      }
    
    // figure out implicit (block boundary) events
    // plugin_engine handles defaults
    std::sort(inner_block.events.accurate.begin(), inner_block.events.accurate.end(), comp);
    for (int a = 0; a < inner_block.events.accurate.size(); a++)
    {
      accurate_event e = _host_block.events.accurate[a];
      bool is_last_event_for_this_param_in_spliced_block = 
        a < inner_block.events.accurate.size() - 1 && 
        inner_block.events.accurate[a].param != inner_block.events.accurate[a + 1].param;
      if (is_last_event_for_this_param_in_spliced_block && e.frame != this_block_frames - 1)
      {
        // just go with linear, we'll see if it pops up in the profiler
        // TODO in theory there should always a "last" event if it matters
        // but better do sample-and-hold in plugin_engine instead of relying on it
        for (int ha = 0; ha < _host_block.events.accurate.size(); ha++)
          if(_host_block.events.accurate[ha].param == inner_block.events.accurate[a].param)
            if (_host_block.events.accurate[ha].frame >= this_block_start + this_block_frames)
            {
              // Found the first event for this param that is within the host block but not in the spliced block.
              // Now linear interpolate.
              auto const& next_host_event_this_param = _host_block.events.accurate[ha];
              int this_event_position_in_host_block = e.frame + this_block_start;
              int next_event_position_in_host_block = next_host_event_this_param.frame;
              int this_event_boundary_position_in_host_block = this_block_start + this_block_frames - 1;
              double this_event_value = e.normalized.value();
              double next_event_value = next_host_event_this_param.normalized.value();
              accurate_event boundary_event;
              boundary_event.param = e.param;
              boundary_event.frame = this_block_frames - 1;
              boundary_event.normalized = normalized_value(
                this_event_value + next_event_value *
                ((float)this_event_boundary_position_in_host_block - (float)this_event_position_in_host_block) /
                ((float)next_event_position_in_host_block - (float)this_event_position_in_host_block));
              break;
            }
      }
    }

    _engine.process();
    _host_block.events.out.insert(_host_block.events.out.begin(), inner_block.events.out.begin(), inner_block.events.out.end());
    _engine.release_block();
  }
}

}
