#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/block_host.hpp>
#include <plugin_base/block_plugin.hpp>

#include <memory>
#include <chrono>
#include <utility>

namespace plugin_base {

class plugin_engine;

// single module audio processors
class module_engine { 
public: 
  virtual void initialize() = 0;
  virtual void process(process_block& block) = 0;
};

// catering to clap
// but also a great way to see if voices are really independent
typedef bool (*
thread_pool_voice_processor)(plugin_engine& engine, void* context);

// global plugin audio processor
class plugin_engine final {

  plugin_desc const _desc;
  plugin_dims const _dims; 

  void* _voice_processor_context = nullptr;
  thread_pool_voice_processor _voice_processor = {};

  float _sample_rate = {};
  std::int64_t _stream_time = {};
  jarray<plain_value, 4> _state = {};
  std::vector<int> _accurate_frames = {};
  jarray<float, 2> _voices_mixdown = {};
  jarray<float, 3> _voice_results = {};
  jarray<float, 4> _voice_cv_state = {};
  jarray<float, 3> _global_cv_state = {};
  jarray<float, 5> _voice_audio_state = {};
  jarray<float, 4> _global_audio_state = {};
  jarray<float, 5> _accurate_automation = {};
  jarray<plain_value, 4> _block_automation = {};
  std::vector<voice_state> _voice_states = {};
  std::unique_ptr<host_block> _host_block = {};
  std::chrono::milliseconds _output_updated_ms = {};
  jarray<std::unique_ptr<module_engine>, 3> _voice_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _input_engines = {};
  jarray<std::unique_ptr<module_engine>, 2> _output_engines = {};
  process_block make_process_block(int voice, int module, int slot, int start_frame, int end_frame);

public:
  void process();
  void deactivate();
  host_block& prepare();
  void process_voice(int v); // public for threadpool
  void activate(int sample_rate, int max_frame_count);

  INF_DECLARE_MOVE_ONLY(plugin_engine);
  explicit plugin_engine(
    std::unique_ptr<plugin_topo>&& topo, 
    thread_pool_voice_processor voice_processor, 
    void* voice_processor_context);

  plugin_desc const& desc() const { return _desc; }
  jarray<plain_value, 4>& state() { return _state; }
  jarray<plain_value, 4> const& state() const { return _state; }
};

}
