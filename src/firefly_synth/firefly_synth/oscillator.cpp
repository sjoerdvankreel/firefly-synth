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

enum { type_off, type_dual };
enum { section_main, section_dual };
enum { dual_type_off, dual_type_sin, dual_type_cos, dual_type_saw };
enum { 
  param_type, param_note, param_cent, 
  param_dual_type_1, param_dual_type_2, param_dual_factor, param_dual_phase, param_dual_mix };
extern int const voice_in_output_pitch_offset;

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{9C9FFCAD-09A5-49E6-A083-482C8A3CF20B}", "Off");
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Dual");
  return result;
}

static std::vector<list_item>
dual_type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{02BC0674-625C-45ED-93CC-953AEC893AE7}", "Off");
  result.emplace_back("{E4248269-63F1-45E3-B6C7-87201486D72E}", "Sin");
  result.emplace_back("{A853F7FC-609D-4BEC-8DEA-C261B5344311}", "Cos");
  result.emplace_back("{913CD63D-C824-4489-B6C7-8BDF24C01381}", "Saw");
  return result;
}

static std::vector<list_item>
dual_factor_items()
{
  std::vector<list_item> result;
  result.emplace_back("{02BC0674-625C-45ED-93CC-953AEC893AE7}", "1X");
  result.emplace_back("{91A59DD0-1BE9-4C74-BA78-17815497ACF8}", "2X");
  result.emplace_back("{059EC6B4-6975-4A1F-939B-B8E9BA1F84C3}", "4X");
  result.emplace_back("{41C80642-5EC8-47A9-89A3-B166CD2A262B}", "8X");
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

private:
  void process_dual(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <class Generator1>
  void process_dual_1(plugin_block& block, cv_matrix_mixdown const* modulation, Generator1 gen1);
  template <class Generator1, class Generator2>
  void process_dual_1_2(plugin_block& block, cv_matrix_mixdown const* modulation, Generator1 gen1, Generator2 gen2);
};

static void
init_minimal(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_type, 0, "Dual");
}

static void
init_default(plugin_state& state)
{
  // TODO
  //state.set_text_at(module_osc, 0, param_type, 0, "Saw");
  //state.set_text_at(module_osc, 0, param_cent, 0, "-10");
  //state.set_text_at(module_osc, 1, param_type, 0, "Saw");
  //state.set_text_at(module_osc, 1, param_cent, 0, "+10");
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
  {
    // TODO
    //result.set_raw_at(module_osc, o, param_type, 0, type_sin);
  }
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
    result.push_back(graph_data(audio, 1.0f, {}));
  }
  engine->process_end();
  return result;
}

static graph_data
render_osc_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  graph_engine_params params = {};
  if(state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step() == type_off) 
    return graph_data(graph_data_type::off, {});
  auto data = render_osc_graphs(state, engine, mapping.module_slot)[mapping.module_slot];
  return graph_data(data.audio(), 1.0f, { "1 Cycle" });
}

module_topo
osc_topo(int section, gui_colors const& colors, gui_position const& pos)
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Oscillator", "Osc", true, true, module_osc, 4),
    make_module_dsp(module_stage::voice, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{FA702356-D73E-4438-8127-0FDD01526B7E}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { gui_dimension::auto_size, 1 } })));

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
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  auto& note = result.params.emplace_back(make_param(
    make_topo_info("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", param_note, 1), 
    make_param_dsp_voice(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 1 }, make_label_none())));
  note.gui.submenu = make_midi_note_submenu();
  note.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& cent = result.params.emplace_back(make_param(
    make_topo_info("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 }, 
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  cent.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& dual = result.sections.emplace_back(make_param_section(section_dual,
    make_topo_tag("{8E776EAB-DAC7-48D6-8C41-29214E338693}", "Dual"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 1, 1 }))));
  dual.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dual; });
  dual.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || vs[0] == type_dual; });
  auto& dual_type_1 = result.params.emplace_back(make_param(
    make_topo_info("{BD753E3C-B84E-4185-95D1-66EA3B27C76B}", "Dual.Type1", "Type1", true, false, param_dual_type_1, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(dual_type_items(), "Sin"),
    make_param_gui_single(section_dual, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  dual_type_1.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dual; });
  auto& dual_type_2 = result.params.emplace_back(make_param(
    make_topo_info("{FB5741F8-7AAB-43D3-AB2B-BCC1EFF838AF}", "Dual.Type2", "Type2", true, false, param_dual_type_2, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(dual_type_items(), ""),
    make_param_gui_single(section_dual, gui_edit_type::autofit_list, { 0, 1 }, make_label_none())));
  dual_type_2.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dual; });
  auto& dual_factor = result.params.emplace_back(make_param(
    make_topo_info("{E102608A-1E0D-4DF2-A903-908141289F4B}", "Dual.Factor", "Factor", true, false, param_dual_factor, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(dual_factor_items(), ""),
    make_param_gui_single(section_dual, gui_edit_type::autofit_list, { 0, 2 }, make_label_none())));
  dual_factor.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dual; });
  auto& dual_phase = result.params.emplace_back(make_param(
    make_topo_info("{52979739-1542-4B0C-BF3D-34CECC77F82F}", "Dual.Phase", "Phase", true, false, param_dual_phase, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_dual, gui_edit_type::hslider, { 0, 3 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dual_phase.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dual; });
  auto& dual_mix = result.params.emplace_back(make_param(
    make_topo_info("{60FAAC91-7F69-4804-AC8B-2C7E6F3E4238}", "Dual.Mix", "Mix", true, false, param_dual_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_dual, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dual_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dual; });

  return result;
}

static float
generate_blep(float phase, float inc)
{
  float b;
  if (phase < inc) return b = phase / inc, (2.0f - b) * b - 1.0f;
  if (phase >= 1.0f - inc) return b = (phase - 1.0f) / inc, (b + 2.0f) * b + 1.0f;
  return 0.0f;
}

static float 
generate_saw(float phase, float inc)
{
  float saw = phase * 2 - 1;
  return saw - generate_blep(phase, inc);
}

void
osc_engine::process(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  // allow custom data for graphs
  if (modulation == nullptr)
    modulation = &get_cv_matrix_mixdown(block, false);

  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  if (type == type_off)
  {
    block.state.own_audio[0][0][0].fill(block.start_frame, block.end_frame, 0.0f);
    block.state.own_audio[0][0][1].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  switch (type)
  {
  case type_dual: process_dual(block, modulation); break;
  default: assert(false); break;
  }
}

void
osc_engine::process_dual(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int dual_type_1 = block_auto[param_dual_type_1][0].step();
  switch (dual_type_1)
  {
  case dual_type_off: process_dual_1(block, modulation, [](float ph, float inc) { return 0.0f; }); break;
  case dual_type_saw: process_dual_1(block, modulation, [](float ph, float inc) { return generate_saw(ph, inc); }); break;
  case dual_type_sin: process_dual_1(block, modulation, [](float ph, float inc) { return std::sin(2.0f * pi32 * ph); }); break;
  case dual_type_cos: process_dual_1(block, modulation, [](float ph, float inc) { return std::cos(2.0f * pi32 * ph); }); break;
  }
}

template <class Generator1> void
osc_engine::process_dual_1(plugin_block& block, cv_matrix_mixdown const* modulation, Generator1 gen1)
{
  auto const& block_auto = block.state.own_block_automation;
  int dual_type_2 = block_auto[param_dual_type_2][0].step();
  switch (dual_type_2)
  {
  case dual_type_off: process_dual_1_2(block, modulation, gen1, [](float ph, float inc) { return 0.0f; }); break;
  case dual_type_saw: process_dual_1_2(block, modulation, gen1, [](float ph, float inc) { return generate_saw(ph, inc); }); break;
  case dual_type_sin: process_dual_1_2(block, modulation, gen1, [](float ph, float inc) { return std::sin(2.0f * pi32 * ph); }); break;
  case dual_type_cos: process_dual_1_2(block, modulation, gen1, [](float ph, float inc) { return std::cos(2.0f * pi32 * ph); }); break;
  }
}

template <class Generator1, class Generator2> void
osc_engine::process_dual_1_2(plugin_block& block, cv_matrix_mixdown const* modulation, Generator1 gen1, Generator2 gen2)
{
  auto const& block_auto = block.state.own_block_automation;
  int note = block_auto[param_note][0].step();
  auto const& cent_curve = *(*modulation)[module_osc][block.module_slot][param_cent][0];
  auto const& voice_pitch_offset_curve = block.voice->all_cv[module_voice_in][0][voice_in_output_pitch_offset][0];
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float cent = block.normalized_to_raw(module_osc, param_cent, cent_curve[f]);
    float freq = pitch_to_freq(note + cent + voice_pitch_offset_curve[f]);
    float inc = std::clamp(freq, 0.0f, block.sample_rate * 0.5f) / block.sample_rate;
    float sample = gen1(_phase, inc);
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
