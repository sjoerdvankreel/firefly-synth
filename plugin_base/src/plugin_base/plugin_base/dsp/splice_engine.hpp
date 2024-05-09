#pragma once

#include <plugin_base/dsp/engine.hpp>

namespace plugin_base {

// block-splice version of engine to reduce memory usage
class plugin_splice_engine final {

  plugin_engine _engine;
  host_block _host_block;
  int _splice_block_size = -1;

  // splice auto and mod separately
  std::vector<accurate_event> _spliced_accurate_automation_events = {};
  std::vector<accurate_event> _spliced_accurate_modulation_events = {};

public:
  PB_PREVENT_ACCIDENTAL_COPY(plugin_splice_engine);

  plugin_splice_engine(
    plugin_desc const* desc, bool graph,
    thread_pool_voice_processor voice_processor,
    void* voice_processor_context);

  void process();
  void deactivate();
  host_block& prepare_block();
  void activate(int max_frame_count);

  plugin_state& state() { return _engine.state(); }
  plugin_state const& state() const { return _engine.state(); }

  void release_block() {}
  void activate_modules() { _engine.activate_modules(); }
  void automation_state_dirty() { _engine.automation_state_dirty(); }
  void set_sample_rate(int sample_rate) { _engine.set_sample_rate(sample_rate); }
  void process_voice(int v, bool threaded) { _engine.process_voice(v, threaded); }
  void mark_param_as_automated(int m, int mi, int p, int pi) { _engine.mark_param_as_automated(m, mi, p, pi); }
};

}
