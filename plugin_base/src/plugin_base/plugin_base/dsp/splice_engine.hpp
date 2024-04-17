#pragma once

#include <plugin_base/dsp/engine.hpp>

namespace plugin_base {

// block-splice version of engine to reduce memory usage
class plugin_splice_engine final {

  plugin_engine _engine;

public:
  PB_PREVENT_ACCIDENTAL_COPY(plugin_splice_engine);

  plugin_splice_engine(
    plugin_desc const* desc, bool graph,
    thread_pool_voice_processor voice_processor,
    void* voice_processor_context);

  // TODO splice
  void process() { _engine.process(); }
  void deactivate() { _engine.deactivate(); }
  void release_block() { _engine.release_block(); }
  host_block& prepare_block() { return _engine.prepare_block(); }
  void activate(int max_frame_count) { _engine.activate(max_frame_count); }
  void process_voice(int v, bool threaded) { _engine.process_voice(v, threaded); }

  plugin_state& state() { return _engine.state(); }
  plugin_state const& state() const { return _engine.state(); }
  void activate_modules() { _engine.activate_modules(); }
  void set_sample_rate(int sample_rate) { _engine.set_sample_rate(sample_rate); }
  void mark_all_params_as_automated(bool automated) { _engine.mark_all_params_as_automated(automated); }
  void mark_param_as_automated(int m, int mi, int p, int pi) { _engine.mark_param_as_automated(m, mi, p, pi); }
};

}
