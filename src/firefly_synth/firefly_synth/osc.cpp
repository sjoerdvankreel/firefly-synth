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
  void process(plugin_block& block) override { process(block, nullptr, nullptr); }
  void process(plugin_block& block, cv_matrix_mixdown const* modulation, jarray<float, 1> const* env_curve);
};

static void
init_default(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_type, 0, "Saw");
  state.set_text_at(module_osc, 0, param_cent, 0, "-10");
  state.set_text_at(module_osc, 1, param_type, 0, "Saw");
  state.set_text_at(module_osc, 1, param_cent, 0, "+10");
}

static std::unique_ptr<tab_menu_handler>
make_osc_routing_menu_handler(plugin_state* state)
{
  auto am_params = make_audio_routing_am_params(state);
  auto cv_params = make_audio_routing_cv_params(state, false);
  auto audio_params = make_audio_routing_audio_params(state, false);
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, std::vector({ audio_params, am_params }));
}

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  graph_engine_params params = {};
  if(state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step() == type_off) return {};
  
  int note = state.get_plain_at(mapping.module_index, mapping.module_slot, param_note, 0).step();
  float cent = state.get_plain_at(mapping.module_index, mapping.module_slot, param_cent, 0).real();
  float freq = pitch_to_freq(note + cent);

  params.bpm = 120;
  params.frame_count = 1000;
  params.midi_key = midi_middle_c;
  params.sample_rate = params.frame_count * freq;

  plugin_block const* block = nullptr;
  graph_engine graph_engine(&state, params);
  for(int i = 0; i <= mapping.module_slot; i++)
    block = graph_engine.process(mapping.module_index, i, [](plugin_block& block) {
      osc_engine engine;
      engine.reset(nullptr);
      jarray<float, 1> env_curve(block.end_frame, 1.0f);
      cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
      engine.process(block, &modulation, &env_curve);
    });

  jarray<float, 2> audio = jarray<float, 2>(block->state.own_audio[0][0]);
  audio[0].push_back(0.0f);
  audio[1].push_back(0.0f);
  return graph_data(audio);
}

module_topo
osc_topo(int section, gui_colors const& colors, gui_position const& pos)
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Oscillator", "Osc", true, module_osc, 4),
    make_module_dsp(module_stage::voice, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{FA702356-D73E-4438-8127-0FDD01526B7E}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { 1 } })));

  result.graph_renderer = render_graph;
  result.default_initializer = init_default;
  result.gui.menu_handler_factory = make_osc_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<osc_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A64046EE-82EB-4C02-8387-4B9EFF69E06A}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));

  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  type.domain.default_selector = [] (int s, int) { return type_items()[s == 0? type_sine: type_off].name; };

  auto& note = result.params.emplace_back(make_param(
    make_topo_info("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", param_note, 1), 
    make_param_dsp_block(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
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
osc_engine::process(plugin_block& block, cv_matrix_mixdown const* modulation, jarray<float, 1> const* env_curve)
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
  if(env_curve == nullptr)
    env_curve = &block.voice->all_cv[module_env][0][0][0];
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
    case type_sine: sample = phase_to_sine(_phase); break;
    case type_saw: sample = (_phase * 2 - 1) - blep(_phase, inc); break;
    default: assert(false); sample = 0; break;
    }
    check_bipolar(sample);
    sample *= (*env_curve)[f];
    block.state.own_audio[0][0][0][f] = sample;
    block.state.own_audio[0][0][1][f] = sample;
    increment_and_wrap_phase(_phase, inc);
  }
}

}
