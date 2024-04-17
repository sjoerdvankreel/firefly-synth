#include <plugin_base/dsp/splice_engine.hpp>

namespace plugin_base {

plugin_splice_engine::
plugin_splice_engine(
  plugin_desc const* desc, bool graph,
  thread_pool_voice_processor voice_processor,
  void* voice_processor_context):
_engine(desc, graph, voice_processor, voice_processor_context) {}

}
