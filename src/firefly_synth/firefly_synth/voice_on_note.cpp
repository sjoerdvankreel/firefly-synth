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

// https://stackoverflow.com/questions/21237905/how-do-i-generate-thread-safe-uniform-random-numbers
static float 
thread_random_next() 
{
  static thread_local std::mt19937 generator;
  std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  auto result = distribution(generator);
  assert(0 <= result && result <= 1);
  return result;
}

class voice_on_note_engine :
public module_engine {
  std::vector<float> _on_note_values;
  std::array<float, on_voice_random_count> _random_values;
  std::vector<module_output_mapping> const _global_outputs;
public:
  void process(plugin_block& block) override;
  void reset(plugin_block const* block) override;
  PB_PREVENT_ACCIDENTAL_COPY(voice_on_note_engine);
  voice_on_note_engine(std::vector<module_output_mapping> const& global_outputs) : 
  _on_note_values(global_outputs.size(), 0.0f), _global_outputs(global_outputs) {}
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
      global_sources.items[i].name, i + 1, 1)));

  module_topo result(make_module(
    make_topo_info("{EF1A4E73-BCAD-4D38-A54E-44B83EF46CB5}", true, "On Note", "On Note", "On Nt", module_voice_on_note, 1),
    make_module_dsp(module_stage::voice, module_output::cv, 0, outputs), make_module_gui_none(section)));
  result.info.description = "Provides a couple of random-on-voice values plus on-note versions of all global modulation sources for the per-voice CV mod matrix.";
  result.engine_factory = [gm = global_sources.mappings](auto const&, int, int) { return std::make_unique<voice_on_note_engine>(gm); };
  return result;
}

void
voice_on_note_engine::process(plugin_block& block)
{
  for(int i = 0; i < on_voice_random_count; i++)
    block.state.own_cv[0][i].fill(block.start_frame, block.end_frame, _random_values[i]);
  for (int i = 0; i < _global_outputs.size(); i++)
    block.state.own_cv[i + 1][0].fill(block.start_frame, block.end_frame, _on_note_values[i]);
}

void 
voice_on_note_engine::reset(plugin_block const* block)
{   
  for(int i = 0; i < on_voice_random_count; i++)
    _random_values[i] = block->graph? (i / (on_voice_random_count - 1.0f)) : thread_random_next();
  for(int i = 0; i < _global_outputs.size(); i++)
  {
    auto const& o = _global_outputs[i];
    _on_note_values[i] = block->state.all_global_cv[o.module_index][o.module_slot][o.output_index][o.output_slot][block->start_frame];
  }
}

}
