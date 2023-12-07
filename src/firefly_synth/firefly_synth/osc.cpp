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

enum { section_main, section_pitch };
enum { type_off, type_sine, type_saw };
enum { scratch_mono, scratch_am, scratch_am_mod, scratch_count };
enum { param_type, param_am, param_note, param_cent, param_pitch, param_pb };

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

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  graph_engine_params params = {};
  if(state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step() == type_off) return {};
  
  int note = state.get_plain_at(mapping.module_index, mapping.module_slot, param_note, 0).step();
  float cent = state.get_plain_at(mapping.module_index, mapping.module_slot, param_cent, 0).real();
  float freq = pitch_to_freq(note_to_pitch(note, cent, midi_middle_c));

  params.bpm = 120;
  params.frame_count = 1000;
  params.midi_key = midi_middle_c;
  params.sample_rate = params.frame_count * freq;

  plugin_block const* block = nullptr;
  graph_engine graph_engine(&state, params);
  for(int i = 0; i <= mapping.module_slot; i++)
    block = graph_engine.process(mapping.module_index, i, [mapping, i, &graph_engine](plugin_block& block) {
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
osc_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos)
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Osc", module_osc, 3), 
    make_module_dsp(module_stage::voice, module_output::audio, scratch_count, {
      make_module_dsp_output(false, make_topo_info("{FA702356-D73E-4438-8127-0FDD01526B7E}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { 1, 1 } })));

  result.graph_renderer = render_graph;
  result.default_initializer = init_default;
  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<osc_engine>(); };
  result.gui.menu_handler_factory = [](plugin_state* state) { return make_audio_routing_menu_handler(state, false); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A64046EE-82EB-4C02-8387-4B9EFF69E06A}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1 }))));

  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  type.domain.default_selector = [] (int s, int) { return type_items()[s == 0? type_sine: type_off].name; };

  auto& am = result.params.emplace_back(make_param(
    make_topo_info("{D03E5C05-E404-4394-BC1F-CE2CD6AAE357}", "AM", param_am, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  am.gui.bindings.enabled.bind_slot([](int s) { return s > 0; });
  am.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  am.dsp.automate_selector = [](int s) { return s > 0? param_automate::automate_modulate : param_automate::none; };

  result.sections.emplace_back(make_param_section(section_pitch,
    make_topo_tag("{4CA0A189-9C44-4260-A5B5-B481527BD04A}", "Pitch"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1 }))));

  auto& note = result.params.emplace_back(make_param(
    make_topo_info("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", param_note, 1), 
    make_param_dsp_block(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_pitch, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));  
  note.gui.submenu = make_midi_note_submenu();
  note.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& cent = result.params.emplace_back(make_param(
    make_topo_info("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_pitch, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  cent.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& pitch = result.params.emplace_back(make_param(
    make_topo_info("{F87BA01D-19CE-4D46-83B6-8E2382D9F601}", "Pitch", param_pitch, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(0, 128, 0, 0, ""),
    make_param_gui_none())); 
  pitch.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& pb = result.params.emplace_back(make_param(
    make_topo_info("{C644681E-0634-4774-B65C-7FFC3B65AADE}", "PB", param_pb, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_none()));
  pb.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

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
  int note = block_auto[param_note][0].step();
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

  auto const& pb_curve = *(*modulation)[module_osc][block.module_slot][param_pb][0];
  auto const& cent_curve = *(*modulation)[module_osc][block.module_slot][param_cent][0];
  auto const& pitch_curve = *(*modulation)[module_osc][block.module_slot][param_pitch][0];
  int pb_range = block.state.all_block_automation[module_master_cv][0][master_cv_param_pb_range][0].step();

  auto& am_scratch = block.state.own_scratch[scratch_am];
  auto& am_mod_scratch = block.state.own_scratch[scratch_am_mod];
  if (block.module_slot == 0)
  {
    am_scratch.fill(block.start_frame, block.end_frame, 1.0f);
    am_mod_scratch.fill(block.start_frame, block.end_frame, 0.0f);
  }
  else
  {
    auto const& am_curve = *(*modulation)[module_osc][block.module_slot][param_am][0];
    am_curve.copy_to(block.start_frame, block.end_frame, am_mod_scratch);
    auto const& am_source = block.voice->all_scratch[module_osc][block.module_slot - 1][scratch_mono];
    am_source.transform_to(block.start_frame, block.end_frame, am_scratch, bipolar_to_unipolar);
  }

  float sample;
  auto& mono_scratch = block.state.own_scratch[scratch_mono];
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float cent = block.normalized_to_raw(module_osc, param_cent, cent_curve[f]);
    float pitch = note_to_pitch(note, cent, block.voice->state.id.key);
    float pitch_mod = block.normalized_to_raw(module_osc, param_pitch, pitch_curve[f]);
    float pitch_pb = block.normalized_to_raw(module_osc, param_pb, pb_curve[f]) * pb_range;
    float inc = pitch_to_freq(pitch + pitch_mod + pitch_pb) / block.sample_rate;
    switch (type)
    {
    case type_sine: sample = phase_to_sine(_phase); break;
    case type_saw: sample = (_phase * 2 - 1) - blep(_phase, inc); break;
    default: assert(false); sample = 0; break;
    }
    check_bipolar(sample);
    sample *= (*env_curve)[f];
    mono_scratch[f] = mix_signal(am_mod_scratch[f], sample, sample * am_scratch[f]);
    block.state.own_audio[0][0][0][f] = mono_scratch[f];
    block.state.own_audio[0][0][1][f] = mono_scratch[f];
    increment_and_wrap_phase(_phase, inc);
  }
}

}
