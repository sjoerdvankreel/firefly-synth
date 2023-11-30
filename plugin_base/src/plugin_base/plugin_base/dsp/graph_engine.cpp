#include <plugin_base/dsp/graph_engine.hpp>

namespace plugin_base {

graph_engine::
~graph_engine()
{ 
  _engine.release_block();
  _engine.deactivate(); 
}

graph_engine::
graph_engine(plugin_state const* state, graph_engine_params const& params):
_engine(&state->desc(), nullptr, nullptr), _state(state), _params(params)
{ 
  _engine.activate(false, _params.sample_rate, _params.frame_count);
  _engine.init_static(state, params.frame_count);
  _audio_in.resize(jarray<int, 1>(2, params.frame_count));
  _audio_out.resize(jarray<int, 1>(2, params.frame_count));

  _audio_in_ptrs[0] = _audio_in[0].data().data();
  _audio_in_ptrs[1] = _audio_in[1].data().data();
  _audio_out_ptrs[0] = _audio_out[0].data().data();
  _audio_out_ptrs[1] = _audio_out[1].data().data();
  _host_block = &_engine.prepare_block();
  _host_block->shared.bpm = _params.bpm;
  _host_block->frame_count = _params.frame_count;
  _host_block->audio_out = _audio_out_ptrs;
  _host_block->shared.audio_in = _audio_in_ptrs;
}

plugin_block const*
graph_engine::process_default(int module_index, int module_slot)
{
  auto factory = _state->desc().plugin->modules[module_index].engine_factory;
  auto module_engine = factory(*_state->desc().plugin, _params.sample_rate, _params.frame_count);
  return process(module_index, module_slot, [engine = module_engine.get()](auto& block) { engine->process(block); });
}

plugin_block const*
graph_engine::process(int module_index, int module_slot, graph_processor processor)
{
  int voice = _state->desc().plugin->modules[module_index].dsp.stage == module_stage::voice ? 0 : -1;
  _last_block = std::make_unique<plugin_block>(
    _engine.make_plugin_block(voice, module_index, module_slot, 0, _params.frame_count));
  if(voice == -1)
    processor(*_last_block.get());
  else
  {
    _last_voice_block = std::make_unique<plugin_voice_block>(
      _engine.make_voice_block(voice, _params.midi_key, _params.voice_release_at));
    _last_block->voice = _last_voice_block.get();
    processor(*_last_block.get());
  }
  return _last_block.get();
}

}