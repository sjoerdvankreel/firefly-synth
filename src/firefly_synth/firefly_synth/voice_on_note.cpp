#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>
#include <random>

using namespace plugin_base;

namespace firefly_synth {

class voice_on_note_engine :
public module_engine {

  std::mt19937 _rand_generator;
  std::uniform_real_distribution<float> _rand_distribution;

  std::vector<float> _on_note_values;
  std::array<float, on_voice_random_count> _random_values;
  std::vector<module_output_mapping> const _global_outputs;
public:
  void process_audio(plugin_block& block) override;
  void reset_audio(plugin_block const* block) override;
  void reset_graph(plugin_block const* block, std::vector<mod_out_custom_state> const& custom_outputs, void* context) override;
  PB_PREVENT_ACCIDENTAL_COPY(voice_on_note_engine);

  // need a different seed for each voice!
  voice_on_note_engine(std::vector<module_output_mapping> const& global_outputs) : 
  _rand_generator(rand()), _rand_distribution(0.0f, 1.0f), _on_note_values(global_outputs.size(), 0.0f), _global_outputs(global_outputs) {}
};

module_topo
voice_on_note_topo(plugin_topo const* topo, int section)
{
  // see also cv_audio_matrix.select_midi_active
  std::vector<module_dsp_output> outputs;
  std::string const on_note_id("{68360340-68B2-4B88-95BD-B1929F240BAA}");
  auto global_sources(make_cv_source_matrix(make_cv_matrix_sources(topo, true)));
  outputs.push_back(make_module_dsp_output(true,
    make_topo_info("{8E4692CE-0A00-4739-BBD2-1E671D24F1B8}", true, "On Note Rnd", "Rnd", "Rnd", 0, on_voice_random_count)));
  for(int i = 0; i < global_sources.items.size(); i++)
    outputs.push_back(make_module_dsp_output(true, make_topo_info(
      global_sources.items[i].id, true,
      global_sources.items[i].name, 
      global_sources.items[i].name,
      global_sources.items[i].name, i + on_voice_random_output_index + 1, 1)));

  module_topo result(make_module(
    make_topo_info("{EF1A4E73-BCAD-4D38-A54E-44B83EF46CB5}", true, "On Note", "On Note", "On Nt", module_voice_on_note, 1),
    make_module_dsp(module_stage::voice, module_output::cv, 0, outputs), make_module_gui_none(section)));
  result.info.description = "Provides a couple of random-on-voice values plus on-note versions of all global modulation sources for the per-voice CV mod matrix.";
  result.engine_factory = [gm = global_sources.mappings](auto const&, int, int) { return std::make_unique<voice_on_note_engine>(gm); };
  return result;
}

void 
voice_on_note_engine::reset_graph(
  plugin_block const* block, 
  std::vector<mod_out_custom_state> const& custom_outputs, 
  void* context)
{
  // do reset_audio then overwrite with live values from process_audio
  reset_audio(block);

  // fix to current live values
  // backwards loop, outputs are sorted, latest-in-time are at the end
  if (custom_outputs.size())
    for (int i = (int)custom_outputs.size() - 1; i >= 0; i--)
      if (custom_outputs[i].module_global == block->module_desc_.info.global)
      {
        int tag = custom_outputs[i].tag_custom;
        float val = custom_outputs[i].value_custom / (float)std::numeric_limits<int>::max();
        if (0 <= tag && tag < on_voice_random_count)
          _random_values[tag] = val;
        else // dealing with on-note-global-lfo-N
          for (int j = 0; j < _global_outputs.size(); j++)
            if (_global_outputs[j].module_index == module_glfo && _global_outputs[j].module_slot == tag - on_voice_random_count)
              _on_note_values[j] = val;
      }
}

void 
voice_on_note_engine::reset_audio(plugin_block const* block)
{   
  for (int i = 0; i < on_voice_random_count; i++)
  {
    // do *not* reset the rand stream itself since that kinda defeats the purpose
    auto next_draw = _rand_distribution(_rand_generator);
    assert(0 <= next_draw && next_draw <= 1);
    _random_values[i] = block->graph ? (i / (on_voice_random_count - 1.0f)) : next_draw;
  }
  for(int i = 0; i < _global_outputs.size(); i++)
  {
    auto const& o = _global_outputs[i];
    _on_note_values[i] = block->state.all_global_cv[o.module_index][o.module_slot][o.output_index][o.output_slot][block->start_frame];
  }
}

void
voice_on_note_engine::process_audio(plugin_block& block)
{
  for (int i = 0; i < on_voice_random_count; i++)
  {
    block.state.own_cv[on_voice_random_output_index][i].fill(block.start_frame, block.end_frame, _random_values[i]);

    if (!block.graph)
      block.push_modulation_output(modulation_output::make_mod_output_custom_state(
        block.voice->state.slot,
        block.module_desc_.info.global,
        i,
        (int)(_random_values[i] * std::numeric_limits<int>::max())));
  }
  for (int i = 0; i < _global_outputs.size(); i++)
  {
    block.state.own_cv[i + on_voice_random_output_index + 1][0].fill(block.start_frame, block.end_frame, _on_note_values[i]);

    if (!block.graph && _global_outputs[i].module_index == module_glfo)
      block.push_modulation_output(modulation_output::make_mod_output_custom_state(
        block.voice->state.slot,
        block.module_desc_.info.global,
        on_voice_random_count + _global_outputs[i].module_slot,
        (int)(_on_note_values[i] * std::numeric_limits<int>::max())));
  }
}

}
