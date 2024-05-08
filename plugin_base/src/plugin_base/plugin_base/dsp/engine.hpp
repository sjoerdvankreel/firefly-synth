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

enum engine_voice_mode {
  engine_voice_mode_poly, // always pick a new voice for new note
  engine_voice_mode_mono, // only pick a new voice when voice count is 0
  engine_voice_mode_release // pick a new voice when voice count is 0 or voice 1 in in the release state
};

// global plugin audio processor
class plugin_engine final {

  // if engine is for graph, 
  // we can forego voice management and just set to 1
  bool const _graph;
  int const _polyphony;

  plugin_dims const _dims;
  plugin_state _state = {};
  plugin_state _block_automation = {};
  jarray<void*, 3> _voice_context = {};
  jarray<void*, 2> _global_context = {};
  jarray<plain_value, 4> _output_values = {};
  jarray<plugin_state, 1> _voice_automation = {};

  float _sample_rate = {};
  int _max_frame_count = {};
  double _cpu_usage = {};
  double _output_updated_sec = {};
  double _block_start_time_sec = {};
  std::int64_t _stream_time = {};
  std::int64_t _blocks_processed = {};
  int _high_cpu_module = {};
  double _high_cpu_module_usage = {};
  jarray<double, 3> _voice_module_process_duration_sec = {};
  jarray<double, 2> _global_module_process_duration_sec = {};

  std::vector<mono_note_state> _mono_note_stream = {};
  jarray<float, 2> _voices_mixdown = {};
  jarray<float, 3> _voice_results = {};
  jarray<float, 6> _voice_cv_state = {};
  jarray<float, 5> _global_cv_state = {};
  jarray<float, 7> _voice_audio_state = {};
  jarray<float, 6> _global_audio_state = {};
  jarray<float, 1> _bpm_automation = {};
  jarray<float, 4> _midi_automation = {};
  jarray<int, 3> _midi_active_selection = {};
  jarray<float, 5> _voice_scratch_state = {};
  jarray<float, 4> _global_scratch_state = {};

  // both automation and modulation
  jarray<int, 4> _param_was_automated = {};
  jarray<float, 5> _accurate_automation = {};
  jarray<cv_filter, 4> _automation_lp_filters = {};
  jarray<block_filter, 4> _automation_lerp_filters = {};
  jarray<float, 4> _automation_state_last_round_end = {};

  // offset wrt _state
  jarray<float, 4> _current_modulation = {};

  block_filter _bpm_filter = {};
  std::vector<int> _midi_was_automated = {};
  std::vector<block_filter> _midi_filters = {};
  std::vector<voice_state> _voice_states = {};
  std::unique_ptr<host_block> _host_block = {};
  
  int _last_note_key = -1;
  int _last_note_channel = -1;
  void* _voice_processor_context = nullptr;
  std::vector<std::thread::id> _voice_thread_ids;
  thread_pool_voice_processor _voice_processor = {};

  jarray<std::unique_ptr<module_engine>, 3> _voice_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _input_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _output_engines = {};

  int find_best_voice_slot();
  void init_automation_from_state();
  void process_voices_single_threaded();
  void automation_sanity_check(int frame_count);

  // Subvoice stuff is for global unison support.
  // In plugin_base we treat global unison voices just like regular polyphonic voices.
  // Or in other words, a polyphonic voice is a global unison voice with subvoice count 1.
  // Subvoice count and index should be used by the plugin to apply detuning etc.
  void activate_voice(note_event const& event, int slot, int sub_voice_count, int sub_voice_index, int frame_count);

public:
  PB_PREVENT_ACCIDENTAL_COPY(plugin_engine);
  plugin_engine(
    plugin_desc const* desc, bool graph,
    thread_pool_voice_processor voice_processor,
    void* voice_processor_context);

  // public for graph_engine
  plugin_block make_plugin_block(
    int voice, int module, int slot,
    int start_frame, int end_frame);
  plugin_voice_block make_voice_block(
    int v, int release_frame, note_id id, 
    int sub_voice_count, int sub_voice_index,
    int last_note_key, int last_note_channel);

  void deactivate();
  void release_block();
  host_block& prepare_block();

  void process();
  void init_bpm_automation(float bpm);
  void voice_block_params_snapshot(int v);
  void process_voice(int v, bool threaded);

  plugin_state& state() { return _state; }
  plugin_state const& state() const { return _state; }

  void activate_modules();
  void automation_state_dirty();
  void activate(int max_frame_count);
  void init_from_state(plugin_state const* state);

  void set_sample_rate(int sample_rate) { _sample_rate = sample_rate; }
  void mark_param_as_automated(int m, int mi, int p, int pi) { _param_was_automated[m][mi][p][pi] = 1; }
};

}
