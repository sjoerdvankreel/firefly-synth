#pragma once

#include <plugin_base/desc/dims.hpp>
#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/block/host.hpp>
#include <plugin_base/dsp/block/plugin.hpp>

#include <memory>
#include <vector>
#include <thread>
#include <utility>

namespace plugin_base {

class plugin_engine;

// single module audio processors
class module_engine { 
public: 
  virtual ~module_engine() {}
  virtual void process(plugin_block& block) = 0;
  virtual void reset(plugin_block const* block) = 0;
};

// catering to clap
// but also a great way to see if voices are really independent
typedef bool (*
thread_pool_voice_processor)(
  plugin_engine& engine, void* context);

// global plugin audio processor
class plugin_engine final {

  plugin_dims const _dims;
  plugin_state _state = {};
  plugin_state _block_automation = {};
  jarray<void*, 3> _voice_context = {};
  jarray<void*, 2> _global_context = {};
  jarray<plain_value, 4> _output_values = {};

  float _sample_rate = {};
  double _cpu_usage = {};
  double _output_updated_sec = {};
  double _block_start_time_sec = {};
  std::int64_t _stream_time = {};

  std::vector<int> _accurate_frames = {};
  jarray<float, 2> _voices_mixdown = {};
  jarray<float, 3> _voice_results = {};
  jarray<float, 6> _voice_cv_state = {};
  jarray<float, 5> _global_cv_state = {};
  jarray<float, 7> _voice_audio_state = {};
  jarray<float, 6> _global_audio_state = {};
  jarray<float, 4> _midi_automation = {};
  jarray<int, 3> _midi_active_selection = {};
  jarray<float, 5> _accurate_automation = {};
  jarray<float, 5> _voice_scratch_state = {};
  jarray<float, 4> _global_scratch_state = {};
  std::vector<int> _midi_was_automated = {};
  std::vector<midi_filter> _midi_filters = {};
  std::vector<voice_state> _voice_states = {};
  std::unique_ptr<host_block> _host_block = {};
  
  void* _voice_processor_context = nullptr;
  std::vector<std::thread::id> _voice_thread_ids;
  thread_pool_voice_processor _voice_processor = {};

  jarray<std::unique_ptr<module_engine>, 3> _voice_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _input_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _output_engines = {};

  void process_voices_single_threaded();
  void init_automation_from_state(int frame_count);

public:
  PB_PREVENT_ACCIDENTAL_COPY(plugin_engine);
  plugin_engine(
    plugin_desc const* desc,
    thread_pool_voice_processor voice_processor,
    void* voice_processor_context);

  // public for graph_engine
  plugin_block make_plugin_block(
    int voice, int module, int slot,
    int start_frame, int end_frame);
  plugin_voice_block make_voice_block(
    int v, int midi_key, int release_frame);

  // per-voice public for threadpool
  void process();
  void process_voice(int v, bool threaded); 

  void deactivate();
  host_block& prepare_block();

  plugin_state& state() { return _state; }
  plugin_state const& state() const { return _state; }

  // set all state and automation to these values
  void init_static(plugin_state const* state, int frame_count);

  void release_block();
  void activate(bool activate_module_engines, int sample_rate, int max_frame_count);
};

}
