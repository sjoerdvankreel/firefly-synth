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

enum { type_off, type_basic };
enum { section_main, section_basic };
enum { 
  param_type, param_note, param_cent, 
  param_basic_sin_on, param_basic_sin_mix, param_basic_saw_on, param_basic_saw_mix,
  param_basic_tri_on, param_basic_tri_mix, param_basic_sqr_on, param_basic_sqr_mix, param_basic_sqr_pwm };
extern int const voice_in_output_pitch_offset;

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{9C9FFCAD-09A5-49E6-A083-482C8A3CF20B}", "Off");
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Basic");
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
  void process_basic(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin> void 
  process_basic_sin(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin, bool Saw> 
  void process_basic_sin_saw(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin, bool Saw, bool Tri> 
  void process_basic_sin_saw_tri(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin, bool Saw, bool Tri, bool Sqr> 
  void process_basic_sin_saw_tri_sqr(plugin_block& block, cv_matrix_mixdown const* modulation);
};

static void
init_minimal(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_type, 0, "Basic");
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
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 }, 
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  cent.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& basic = result.sections.emplace_back(make_param_section(section_basic,
    make_topo_tag("{8E776EAB-DAC7-48D6-8C41-29214E338693}", "Basic"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));
  basic.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  basic.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || vs[0] == type_basic; });
  auto& basic_sin_on = result.params.emplace_back(make_param(
    make_topo_info("{BD753E3C-B84E-4185-95D1-66EA3B27C76B}", "Sin.On", "On", true, false, param_basic_sin_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 0 }, make_label_none())));
  basic_sin_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  auto& basic_sin_mix = result.params.emplace_back(make_param(
    make_topo_info("{60FAAC91-7F69-4804-AC8B-2C7E6F3E4238}", "Sin.Mix", "Sin", true, false, param_basic_sin_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  basic_sin_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_sin_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  auto& basic_saw_on = result.params.emplace_back(make_param(
    make_topo_info("{A31C1E92-E7FF-410F-8466-7AC235A95BDB}", "Saw.On", "On", true, false, param_basic_saw_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 2 }, make_label_none())));
  basic_saw_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  auto& basic_saw_mix = result.params.emplace_back(make_param(
    make_topo_info("{A459839C-F78E-4871-8494-6D524F00D0CE}", "Saw.Mix", "Saw", true, false, param_basic_saw_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  basic_saw_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_saw_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  auto& basic_tri_on = result.params.emplace_back(make_param(
    make_topo_info("{F2B92036-ED14-4D88-AFE3-B83C1AAE5E76}", "Tri.On", "On", true, false, param_basic_tri_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 4 }, make_label_none())));
  basic_tri_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  auto& basic_tri_mix = result.params.emplace_back(make_param(
    make_topo_info("{88F88506-5916-4668-BD8B-5C35D01D1147}", "Tri.Mix", "Tri", true, false, param_basic_tri_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::knob, { 0, 5 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  basic_tri_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_tri_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  auto& basic_sqr_on = result.params.emplace_back(make_param(
    make_topo_info("{C3AF1917-64FD-481B-9C21-3FE6F8D039C4}", "Sqr.On", "On", true, false, param_basic_sqr_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 6 }, make_label_none())));
  basic_sqr_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  auto& basic_sqr_mix = result.params.emplace_back(make_param(
    make_topo_info("{B133B0E6-23DC-4B44-AA3B-6D04649271A4}", "Sqr.Mix", "Sqr", true, false, param_basic_sqr_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::knob, { 0, 7 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  basic_sqr_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_sqr_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  auto& basic_sqr_pwm = result.params.emplace_back(make_param(
    make_topo_info("{57A231B9-CCC7-4881-885E-3244AE61107C}", "Sqr.PWM", "PWM", true, false, param_basic_sqr_pwm, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::hslider, { 0, 8 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  basic_sqr_pwm.gui.bindings.enabled.bind_params({ param_type, param_basic_sqr_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });

  return result;
}

// https://www.kvraudio.com/forum/viewtopic.php?t=375517
static inline float
generate_blep(float phase, float inc)
{
  float b;
  if (phase < inc) return b = phase / inc, (2.0f - b) * b - 1.0f;
  if (phase >= 1.0f - inc) return b = (phase - 1.0f) / inc, (b + 2.0f) * b + 1.0f;
  return 0.0f;
}

static inline float
generate_saw(float phase, float inc)
{
  float saw = phase * 2 - 1;
  return saw - generate_blep(phase, inc);
}

// https://dsp.stackexchange.com/questions/54790/polyblamp-anti-aliasing-in-c
static float
generate_blamp(float phase, float increment)
{
  float y = 0.0f;
  if (!(0.0f <= phase && phase < 2.0f * increment))
    return y * increment / 15;
  float x = phase / increment;
  float u = 2.0f - x;
  u *= u * u * u * u;
  y -= u;
  if (phase >= increment)
    return y * increment / 15;
  float v = 1.0f - x;
  v *= v * v * v * v;
  y += 4 * v;
  return y * increment / 15;
}

static float
generate_triangle(float phase, float increment)
{
  float const triangle_scale = 0.9f;
  phase = phase + 0.75f;
  phase -= std::floor(phase);
  float result = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
  result += generate_blamp(phase, increment);
  result += generate_blamp(1.0f - phase, increment);
  phase += 0.5f;
  phase -= std::floor(phase);
  result += generate_blamp(phase, increment);
  result += generate_blamp(1.0f - phase, increment);
  return result * triangle_scale;
}

static float
generate_sqr(float phase, float increment, float pwm)
{
  float const min_pw = 0.05f;
  float pw = (min_pw + (1.0f - min_pw) * pwm) * 0.5f;
  float saw1 = generate_saw(phase, increment);
  float saw2 = generate_saw(phase + pw - std::floor(phase + pw), increment);
  return (saw1 - saw2) * 0.5f;
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
  case type_basic: process_basic(block, modulation); break;
  default: assert(false); break;
  }
}

void
osc_engine::process_basic(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sin = block_auto[param_basic_sin_on][0].step();
  if(sin) process_basic_sin<true>(block, modulation);
  else process_basic_sin<false>(block, modulation);
}

template <bool Sin> void
osc_engine::process_basic_sin(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool saw = block_auto[param_basic_saw_on][0].step();
  if (saw) process_basic_sin_saw<Sin, true>(block, modulation);
  else process_basic_sin_saw<Sin, false>(block, modulation);
}

template <bool Sin, bool Saw> void
osc_engine::process_basic_sin_saw(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool tri = block_auto[param_basic_tri_on][0].step();
  if (tri) process_basic_sin_saw_tri<Sin, Saw, true>(block, modulation);
  else process_basic_sin_saw_tri<Sin, Saw, false>(block, modulation);
}

template <bool Sin, bool Saw, bool Tri> void
osc_engine::process_basic_sin_saw_tri(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sqr = block_auto[param_basic_sqr_on][0].step();
  if (sqr) process_basic_sin_saw_tri_sqr<Sin, Saw, Tri, true>(block, modulation);
  else process_basic_sin_saw_tri_sqr<Sin, Saw, Tri, false>(block, modulation);
}

template <bool Sin, bool Saw, bool Tri, bool Sqr> void
osc_engine::process_basic_sin_saw_tri_sqr(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  int count = 0;
  if constexpr (Sin) count++;
  if constexpr (Saw) count++;
  if constexpr (Tri) count++;
  if constexpr (Sqr) count++;

  if (count == 0)
  {
    block.state.own_audio[0][0][0].fill(block.start_frame, block.end_frame, 0.0f);
    block.state.own_audio[0][0][1].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  auto const& block_auto = block.state.own_block_automation;
  int note = block_auto[param_note][0].step();

  auto const& cent_curve = *(*modulation)[module_osc][block.module_slot][param_cent][0];
  auto const& sin_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sin_mix][0];
  auto const& saw_curve = *(*modulation)[module_osc][block.module_slot][param_basic_saw_mix][0];
  auto const& tri_curve = *(*modulation)[module_osc][block.module_slot][param_basic_tri_mix][0];
  auto const& sqr_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sqr_mix][0];
  auto const& pwm_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sqr_pwm][0];
  auto const& voice_pitch_offset_curve = block.voice->all_cv[module_voice_in][0][voice_in_output_pitch_offset][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float sample = 0;
    float cent = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_cent, cent_curve[f]);
    float freq = pitch_to_freq(note + cent + voice_pitch_offset_curve[f]);
    float inc = std::clamp(freq, 0.0f, block.sample_rate * 0.5f) / block.sample_rate;
    
    if constexpr (Sin) sample += std::sin(2.0f * pi32 * _phase) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_sin_mix, sin_curve[f]);
    if constexpr (Saw) sample += generate_saw(_phase, inc) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_saw_mix, saw_curve[f]);
    if constexpr (Tri) sample += generate_triangle(_phase, inc) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_tri_mix, tri_curve[f]);
    if constexpr (Sqr) sample += generate_sqr(_phase, inc, pwm_curve[f]) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_sqr_mix, sqr_curve[f]);

    check_bipolar(sample / count);
    block.state.own_audio[0][0][0][f] = sample / count;
    block.state.own_audio[0][0][1][f] = sample / count;
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
