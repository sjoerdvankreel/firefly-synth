#include <plugin_base/dsp/graph_engine.hpp>

namespace plugin_base {

module_graph_engine::
~module_graph_engine() 
{ 
  _engine.release_block();
  _engine.deactivate(); 
}

module_graph_engine::
module_graph_engine(plugin_state const* state, module_graph_params const& params):
_engine(&state->desc(), nullptr, nullptr), _audio(), _state(state), _params(params)
{ 
  _engine.activate(_params.sample_rate, _params.frame_count); 
  _engine.init_static(state, params.frame_count);
  _audio.resize(jarray<int, 1>(4, params.frame_count));

  _audio_in[0] = _audio[0].data().data();
  _audio_in[1] = _audio[1].data().data();
  _audio_out[0] = _audio[2].data().data();
  _audio_out[1] = _audio[3].data().data();
  _host_block = &_engine.prepare_block();
  _host_block->audio_out = _audio_out;
  _host_block->shared.audio_in = _audio_in;
  _host_block->shared.bpm = _params.bpm;
  _host_block->frame_count = _params.frame_count;
}

void 
module_graph_engine::process(int module_index, int module_slot, graph_processor processor)
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
}

}