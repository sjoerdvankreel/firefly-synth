#include <plugin_base/dsp/splice_engine.hpp>

namespace plugin_base {

// baseline = ~250mb, with each 64 samples is ~100mb of extra data
// should investigate

// with all route count set to 4: baseline = 220
// each extra 64 samples = 100

// with polyphony set to 4: baseline = 80
// each extra 64 samples = 4

// ok so its not the automation buffers since those are not per-voice
// maybe the oversamplers ?

// with no changes but oversampling: 
// baseline = 220
// each extra 64 samples = 100

// don't activate() and dont activate_modules()
// baseline = 50 
// extra 64 = 0

// activate() but dont activate_modules()
// baseline = 60
// extra 64 = 20

// ok so it's one of the per-voice modules
// investigate more

// 2 osc and 2 vfx
// baseline = 150
// extra 64  = 70

// no changes but max_osc_unison_voices = 2
// baseline = 180
// extra 64 = 60

// no changes but env = 2
// base = 220
// extra 64  = 100

static int const default_block_size = 64;
  
plugin_splice_engine::
plugin_splice_engine(
  plugin_desc const* desc, bool graph, int block_size,
  thread_pool_voice_processor voice_processor,
  void* voice_processor_context):
_engine(desc, graph, voice_processor, voice_processor_context),
_splice_block_size(block_size == default_splice_block_size? default_block_size: block_size) {}

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
}

void 
plugin_splice_engine::activate(int max_frame_count)
{
  _engine.activate(_splice_block_size);
  _host_block.events.activate(state().desc().param_count, state().desc().midi_count, state().desc().plugin->audio_polyphony, max_frame_count);
}

void
plugin_splice_engine::process()
{

}

}
