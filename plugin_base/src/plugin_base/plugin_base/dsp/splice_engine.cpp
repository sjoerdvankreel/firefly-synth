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

    for (int b = 0; b < _host_block.events.block.size(); b++)
      inner_block.events.block.push_back(_host_block.events.block[b]);
    for(int m = 0; m < _host_block.events.midi.size(); m++)
      if(_host_block.events.midi[m].frame >= this_block_start && _host_block.events.midi[m].frame < this_block_start + this_block_frames)
      {
        midi_event e = _host_block.events.midi[m];
        e.frame -= this_block_start;
        inner_block.events.midi.push_back(e);
      }
    for (int a = 0; a < _host_block.events.accurate.size(); a++)
      if (_host_block.events.accurate[a].frame >= this_block_start && _host_block.events.accurate[a].frame < this_block_start + this_block_frames)
      {
        accurate_event e = _host_block.events.accurate[a];
        e.frame -= this_block_start;
        inner_block.events.accurate.push_back(e);
      }
    for (int n = 0; n < _host_block.events.notes.size(); n++)
      if (_host_block.events.notes[n].frame >= this_block_start && _host_block.events.notes[n].frame < this_block_start + this_block_frames)
      {
        note_event e = _host_block.events.notes[n];
        e.frame -= this_block_start;
        inner_block.events.notes.push_back(e);
      }

    _engine.process();
    _host_block.events.out.insert(_host_block.events.out.begin(), inner_block.events.out.begin(), inner_block.events.out.end());
    _engine.release_block();
  }
}

}
