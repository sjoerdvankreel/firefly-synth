#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { section_main };
enum { type_off, type_sine, type_saw };
enum { param_type, param_note, param_cent };

extern int const voice_in_output_pitch_offset;

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{9C9FFCAD-09A5-49E6-A083-482C8A3CF20B}", "Off");
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Sine");
  result.emplace_back("{E41F2F4B-7E80-4791-8E9C-CCE72A949DB6}", "Saw");
  return result;
}

class osc_engine:
public module_engine {
  float _phase;

public:
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(osc_engine);
  void reset(plugin_block const*) override { _phase = 0; }
  void process(plugin_block& block) override { process(block, nullptr); }
  void process(plugin_block& block, cv_matrix_mixdown const* modulation);
};

static void
init_minimal(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_type, 0, "Sine");
}

static void
init_default(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_type, 0, "Saw");
  state.set_text_at(module_osc, 0, param_cent, 0, "-10");
  state.set_text_at(module_osc, 1, param_type, 0, "Saw");
  state.set_text_at(module_osc, 1, param_cent, 0, "+10");
}

static std::unique_ptr<module_tab_menu_handler>
make_osc_routing_menu_handler(plugin_state* state)
{
  auto am_params = make_audio_routing_am_params(state);
  auto cv_params = make_audio_routing_cv_params(state, false);
  auto audio_params = make_audio_routing_audio_params(state, false);
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, std::vector({ audio_params, am_params }));
}

plugin_state
prepare_osc_state_for_am_graph(plugin_state const& state)
{
  // todo disable unison etc
  plugin_state result(&state.desc(), false);
  result.copy_from(state.state());
  for (int o = 0; o < state.desc().plugin->modules[module_osc].info.slot_count; o++)
    result.set_raw_at(module_osc, o, param_type, 0, type_sine);
  return result;
}

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result = {};
  result.bpm = 120;
  result.max_frame_count = 1000;
  result.midi_key = midi_middle_c;
  return result;
}

std::unique_ptr<graph_engine>
make_osc_graph_engine(plugin_desc const* desc)
{
  auto params = make_graph_engine_params();
  return std::make_unique<graph_engine>(desc, params);
}

std::vector<graph_data>
render_osc_graphs(plugin_state const& state, graph_engine* engine, int slot)
{
  std::vector<graph_data> result;
  int note = state.get_plain_at(module_osc, slot, param_note, 0).step();
  float cent = state.get_plain_at(module_osc, slot, param_cent, 0).real();
  float freq = pitch_to_freq(note + cent);
  
  plugin_block const* block = nullptr;
  auto const params = make_graph_engine_params();
  int sample_rate = params.max_frame_count * freq;
  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  engine->process_default(module_am_matrix, 0);
  for (int i = 0; i <= slot; i++)
  {
    block = engine->process(module_osc, i, [](plugin_block& block) {
      osc_engine engine;
      engine.reset(&block);
      cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
      engine.process(block, &modulation);
    });
    jarray<float, 2> audio = jarray<float, 2>(block->state.own_audio[0][0]);
    result.push_back(graph_data(audio, {}));
  }
  engine->process_end();
  return result;
}

static graph_data
render_osc_graph(plugin_state const& state, graph_engine* engine, param_topo_mapping const& mapping)
{
  graph_engine_params params = {};
  if(state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step() == type_off) 
    return graph_data(graph_data_type::off, {});
  return render_osc_graphs(state, engine, mapping.module_slot)[mapping.module_slot];
}

module_topo
osc_topo(int section, gui_colors const& colors, gui_position const& pos)
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Oscillator", "Osc", true, module_osc, 4),
    make_module_dsp(module_stage::voice, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{FA702356-D73E-4438-8127-0FDD01526B7E}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { 1 } })));

  result.minimal_initializer = init_minimal;
  result.default_initializer = init_default;
  result.graph_renderer = render_osc_graph;
  result.graph_engine_factory = make_osc_graph_engine;
  result.gui.menu_handler_factory = make_osc_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<osc_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A64046EE-82EB-4C02-8387-4B9EFF69E06A}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", param_type, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));

  auto& note = result.params.emplace_back(make_param(
    make_topo_info("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", param_note, 1), 
    make_param_dsp_voice(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 1 }, gui_label_contents::name, make_label_none())));
  note.gui.submenu = make_midi_note_submenu();
  note.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& cent = result.params.emplace_back(make_param(
    make_topo_info("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 }, gui_label_contents::name,
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  cent.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  return result;
}

static float
blep(float phase, float inc)
{
  float b;
  if (phase < inc) return b = phase / inc, (2.0f - b) * b - 1.0f;
  if (phase >= 1.0f - inc) return b = (phase - 1.0f) / inc, (b + 2.0f) * b + 1.0f;
  return 0.0f;
}

void
osc_engine::process(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  if (type == type_off)
  {
    block.state.own_audio[0][0][0].fill(block.start_frame, block.end_frame, 0.0f);
    block.state.own_audio[0][0][1].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }
  
  // allow custom data for graphs
  if(modulation == nullptr)
    modulation = &get_cv_matrix_mixdown(block, false);

  float sample;
  int note = block_auto[param_note][0].step();
  auto const& cent_curve = *(*modulation)[module_osc][block.module_slot][param_cent][0];
  auto const& voice_pitch_offset_curve = block.voice->all_cv[module_voice_in][0][voice_in_output_pitch_offset][0];
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float cent = block.normalized_to_raw(module_osc, param_cent, cent_curve[f]);
    float freq = pitch_to_freq(note + cent + voice_pitch_offset_curve[f]);
    float inc = std::clamp(freq, 0.0f, block.sample_rate * 0.5f) / block.sample_rate;
    switch (type)
    {
    case type_sine: sample = std::sin(2.0f * pi32 * _phase); break;
    case type_saw: sample = (_phase * 2 - 1) - blep(_phase, inc); break;
    default: assert(false); sample = 0; break;
    }
    check_bipolar(sample);
    block.state.own_audio[0][0][0][f] = sample;
    block.state.own_audio[0][0][1][f] = sample;
    increment_and_wrap_phase(_phase, inc);
  }

  // apply AM/RM afterwards (since we can self-modulate, so modulator takes *our* own_audio into account)
  auto& modulator = get_am_matrix_modulator(block);
  auto const& modulated = modulator.modulate(block, block.module_slot, modulation);
  for(int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
      block.state.own_audio[0][0][c][f] = modulated[c][f];
}

}
