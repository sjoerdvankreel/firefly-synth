#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/desc/desc.hpp>
#include <plugin_base/desc/dims.hpp>
#include <plugin_base/block/host.hpp>
#include <plugin_base/block/plugin.hpp>

#include <memory>
#include <vector>
#include <thread>
#include <utility>

namespace plugin_base {

class plugin_engine;

// single module audio processors
class module_engine { 
public: 
  virtual void initialize() = 0;
  virtual void process(plugin_block& block) = 0;
};

// catering to clap
// but also a great way to see if voices are really independent
typedef bool (*
thread_pool_voice_processor)(
  plugin_engine& engine, void* context);

// global plugin audio processor
class plugin_engine final {

  plugin_desc const _desc;
  plugin_dims const _dims;

  float _sample_rate = {};
  double _cpu_usage = {};
  double _output_updated_sec = {};
  double _block_start_time_sec = {};
  std::int64_t _stream_time = {};

  std::vector<int> _accurate_frames = {};
  jarray<float, 2> _voices_mixdown = {};
  jarray<float, 3> _voice_results = {};
  jarray<float, 5> _voice_cv_state = {};
  jarray<float, 4> _global_cv_state = {};
  jarray<float, 6> _voice_audio_state = {};
  jarray<float, 5> _global_audio_state = {};
  jarray<float, 5> _accurate_automation = {};
  jarray<plain_value, 4> _plugin_state = {};
  jarray<plain_value, 4> _block_automation = {};
  std::vector<voice_state> _voice_states = {};
  std::unique_ptr<host_block> _host_block = {};
  
  void* _voice_processor_context = nullptr;
  std::vector<std::thread::id> _voice_thread_ids;
  thread_pool_voice_processor _voice_processor = {};

  jarray<std::unique_ptr<module_engine>, 3> _voice_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _input_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _output_engines = {};

  plugin_block make_plugin_block(
    int voice, int module, int slot, 
    int start_frame, int end_frame);
  int process_voices_single_threaded();

public:
  INF_DECLARE_MOVE_ONLY(plugin_engine);
  plugin_engine(
    std::unique_ptr<plugin_topo>&& topo,
    thread_pool_voice_processor voice_processor,
    void* voice_processor_context);

  // per-voice public for threadpool
  void process();
  void process_voice(int v, bool threaded); 

  void deactivate();
  void release_block();
  host_block& prepare_block();
  void activate(int sample_rate, int max_frame_count);

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& plugin_state() { return _plugin_state; }
  jarray<plain_value, 4> const& plugin_state() const { return _plugin_state; }
};

}
