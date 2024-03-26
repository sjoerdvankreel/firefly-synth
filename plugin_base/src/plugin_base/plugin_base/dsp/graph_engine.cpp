#include <plugin_base/dsp/graph_engine.hpp>

namespace plugin_base {

graph_engine::
graph_engine(plugin_desc const* desc, graph_engine_params const& params):
_engine(desc, true, nullptr, nullptr), _desc(desc), _params(params)
{ 
  _engine.activate(_params.max_frame_count);
  _engine.init_bpm_automation(params.bpm);
  _audio_in.resize(jarray<int, 1>(2, params.max_frame_count));
  _audio_out.resize(jarray<int, 1>(2, params.max_frame_count));
  _audio_in_ptrs[0] = _audio_in[0].data().data();
  _audio_in_ptrs[1] = _audio_in[1].data().data();
  _audio_out_ptrs[0] = _audio_out[0].data().data();
  _audio_out_ptrs[1] = _audio_out[1].data().data();  
}

void 
graph_engine::process_end()
{
  _engine.release_block();
  _host_block = nullptr;
  _sample_rate = -1;
  _voice_release_at = -1;
}

void 
graph_engine::process_begin(plugin_state const* state, int sample_rate, int frame_count, int voice_release_at)
{
  assert(sample_rate > 0);
  assert(0 < frame_count && frame_count <= _params.max_frame_count);
  _engine.set_sample_rate(sample_rate);
  _sample_rate = sample_rate;
  _voice_release_at = voice_release_at;
  _host_block = &_engine.prepare_block();
  _host_block->frame_count = frame_count;
  _host_block->shared.bpm = _params.bpm;
  _host_block->audio_out = _audio_out_ptrs;
  _host_block->shared.audio_in = _audio_in_ptrs;
  _engine.init_from_state(state);
}

plugin_block const*
graph_engine::process_default(int module_index, int module_slot)
{
  assert(_sample_rate > 0);
  assert(_host_block != nullptr);
  module_engine* engine = nullptr;
  auto& slot_map = _activated[module_index];
  auto const& module = _desc->plugin->modules[module_index];
  if (slot_map.find(module_slot) == slot_map.end())
  {
    auto module_engine = module.engine_factory(*_desc->plugin, _sample_rate, _host_block->frame_count);
    engine = module_engine.get();
    slot_map[module_slot] = std::move(module_engine);
  } else
    engine = slot_map[module_slot].get();
  bool voice = module.dsp.stage == module_stage::voice;
  return process(module_index, module_slot, [engine, voice](auto& block) { 
    engine->reset(&block);
    engine->process(block); 
  });
}

plugin_block const*
graph_engine::process(int module_index, int module_slot, graph_processor processor)
{
  int voice = _desc->plugin->modules[module_index].dsp.stage == module_stage::voice ? 0 : -1;
  _last_block = std::make_unique<plugin_block>(
    _engine.make_plugin_block(voice, module_index, module_slot, 0, _host_block->frame_count));
  if(voice == -1)
    processor(*_last_block.get());
  else
  {
    note_id id;
    id.id = 0;
    id.channel = 0;
    id.key = _params.midi_key;
    _engine.voice_block_params_snapshot(voice);
    _last_voice_block = std::make_unique<plugin_voice_block>(
      _engine.make_voice_block(voice, _voice_release_at, id, 1, 0, -1, -1));
    _last_block->voice = _last_voice_block.get();
    processor(*_last_block.get());
  }
  return _last_block.get();
}

}