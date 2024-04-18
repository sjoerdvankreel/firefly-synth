#include <plugin_base/dsp/splice_engine.hpp>

namespace plugin_base {

// baseline = ~250mb, with each 64 samples is ~100mb of extra data
// should investigate
static int const default_block_size = 128;

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
