#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/oversampler.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/svf.hpp>
#include <firefly_synth/synth.hpp>
#include <firefly_synth/waves.hpp>
#include <firefly_synth/noise_static.hpp>

#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { type_off, type_basic, type_dsf, type_kps1, type_kps2, type_static };
enum { rand_svf_lpf, rand_svf_hpf, rand_svf_bpf, rand_svf_bsf, rand_svf_peq };
enum { section_main, section_basic, section_dsf, section_rand, section_sync_gain, section_uni };

enum {
  param_type, param_note, param_cent, param_pitch, param_pb,
  param_basic_sin_on, param_basic_sin_mix, param_basic_saw_on, param_basic_saw_mix,
  param_basic_tri_on, param_basic_tri_mix, param_basic_sqr_on, param_basic_sqr_mix, param_basic_sqr_pw,
  param_dsf_parts, param_dsf_dist, param_dsf_dcy,
  param_rand_svf, param_rand_freq, param_rand_res, param_rand_seed, param_rand_rate, // shared k+s/noise
  param_kps_fdbk, param_kps_stretch, param_kps_mid,
  param_gain, param_hard_sync, param_hard_sync_semis, param_hard_sync_xover,
  param_uni_voices, param_uni_phase, param_uni_dtn, param_uni_sprd };

extern int const voice_in_output_pitch_offset;
extern int const osc_param_type = param_type;
extern int const osc_param_uni_voices = param_uni_voices;

static bool constexpr is_kps(int type) 
{ return type == type_kps1 || type == type_kps2; }
static bool constexpr is_random(int type)
{ return type == type_static || is_kps(type); }
static bool can_do_phase(int type)
{ return type == type_basic || type == type_dsf; }
static bool can_do_pitch(int type)
{ return type == type_basic || type == type_dsf || is_kps(type); }

static void 
get_oversmp_info(plugin_block const& block, int& stages, int& factor)
{
  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  stages = block.state.all_block_automation[module_voice_in][0][voice_in_param_oversmp][0].step();
  factor = 1 << stages;
  if (!can_do_phase(type))
  {
    stages = 0;
    factor = 1;
  }
}

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{9C9FFCAD-09A5-49E6-A083-482C8A3CF20B}", "Off");
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Basic");
  result.emplace_back("{5DDB5617-85CC-4BE7-8295-63184BA56191}", "DSF");
  result.emplace_back("{E6814747-6CEE-47DA-9878-890D0A5DC5C7}", "K+S1");
  result.emplace_back("{43DC7825-E437-4792-88D5-6E76241493A1}", "K+S2");
  result.emplace_back("{8B81D211-2A23-4D5D-89B0-24DA3B7D7E2C}", "Static");
  return result;
}

static std::vector<list_item>
random_svf_items()
{
  std::vector<list_item> result;
  result.emplace_back("{E4193E9C-3305-42AE-90D4-A9A5554E43EA}", "LPF");
  result.emplace_back("{D51180AF-5D49-4BB6-BE73-73EF753A15A8}", "HPF");
  result.emplace_back("{F2DCD276-E111-4A63-8701-6751438A1FAA}", "BPF");
  result.emplace_back("{CC4012D9-272E-4E33-96EB-AF1ADBF0E879}", "BSF");
  result.emplace_back("{C4025CB0-5B1A-4B5D-A293-CAD380F264FA}", "PEQ");
  return result;
}

// https://ristoid.net/modular/fm_variants.html
// https://www.verklagekasper.de/synths/dsfsynthesis/dsfsynthesis.html
// https://blog.demofox.org/2016/06/16/synthesizing-a-pluked-string-sound-with-the-karplus-strong-algorithm/
// https://github.com/marcociccone/EKS-string-generator/blob/master/Extended%20Karplus%20Strong%20Algorithm.ipynb
class osc_engine:
public module_engine {

  // basic and dsf
  float _ref_phases[max_osc_unison_voices];
  float _sync_phases[max_osc_unison_voices];
  // for lerp hardsync
  int _unsync_samples[max_osc_unison_voices];
  float _unsync_phases[max_osc_unison_voices];

  // oversampler and pointers into upsampled buffers
  oscillator_context _context = {};
  oversampler<max_osc_unison_voices + 1> _oversampler;

  // random (static and k+s)
  std::array<dc_filter, max_osc_unison_voices> _random_dcs = {};

  // static
  std::array<static_noise, max_osc_unison_voices> _static_noises = {};
  std::array<state_var_filter, max_osc_unison_voices> _static_svfs = {};

  // kps
  int _kps_max_length = {};
  bool _kps_initialized = false;
  std::array<int, max_osc_unison_voices> _kps_freqs = {};
  std::array<int, max_osc_unison_voices> _kps_lengths = {};
  std::array<int, max_osc_unison_voices> _kps_positions = {};
  std::array<std::vector<float>, max_osc_unison_voices> _kps_lines = {};

public:
  PB_PREVENT_ACCIDENTAL_COPY(osc_engine);
  osc_engine(int max_frame_count, float sample_rate);

  void reset(plugin_block const*) override;
  void process(plugin_block& block) override { process<false>(block, nullptr); }
  
  template <bool Graph> 
  void process(plugin_block& block, cv_audio_matrix_mixdown const* modulation);

private:

  template <int SVFType>
  float generate_static(int voice, float sr, float freq_hz, float res, int seed, float rate_hz);
  template <bool AutoFdbk>
  float generate_kps(int voice, float sr, float freq, float fdbk, float stretch, float mid_freq);

  void init_kps(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph> void process_dsf(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph> void process_basic(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph> void process_static(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  
  template <bool Graph, bool Sin> void
  process_basic_sin(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph, bool Sin, bool Saw>
  void process_basic_sin_saw(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph, bool Sin, bool Saw, bool Tri>
  void process_basic_sin_saw_tri(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph, bool Sin, bool Saw, bool Tri, bool Sqr>
  void process_basic_sin_saw_tri_sqr(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
  template <bool Graph, bool Sin, bool Saw, bool Tri, bool Sqr, bool DSF, bool Sync, bool KPS, bool KPSAutoFdbk, bool Static, int StaticSVFType>
  void process_unison(plugin_block& block, cv_audio_matrix_mixdown const* modulation);
};

static void
init_minimal(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_type, 0, "Basic");
}

static void
init_default(plugin_state& state)
{
  state.set_text_at(module_osc, 0, param_cent, 0, "-10");
  state.set_text_at(module_osc, 0, param_type, 0, "Basic");
  state.set_text_at(module_osc, 0, param_basic_sin_on, 0, "Off");
  state.set_text_at(module_osc, 0, param_basic_saw_on, 0, "On");
  state.set_text_at(module_osc, 1, param_cent, 0, "+10");
  state.set_text_at(module_osc, 1, param_type, 0, "Basic");
  state.set_text_at(module_osc, 1, param_basic_sin_on, 0, "Off");
  state.set_text_at(module_osc, 1, param_basic_saw_on, 0, "On");
}

static std::unique_ptr<module_tab_menu_handler>
make_osc_routing_menu_handler(plugin_state* state)
{
  auto cv_params = make_audio_routing_cv_params(state, false);
  auto osc_mod_params = make_audio_routing_osc_mod_params(state);
  auto audio_params = make_audio_routing_audio_params(state, false);
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, std::vector({ audio_params, osc_mod_params }));
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
render_osc_graphs(plugin_state const& state, graph_engine* engine, int slot, bool for_osc_osc_matrix)
{
  std::vector<graph_data> result;
  int note = state.get_plain_at(module_osc, slot, param_note, 0).step();
  int type = state.get_plain_at(module_osc, slot, param_type, 0).step();
  float cent = state.get_plain_at(module_osc, slot, param_cent, 0).real();
  float gain = state.get_plain_at(module_osc, slot, param_gain, 0).real();
  float freq = pitch_to_freq(note + cent);
  
  plugin_block const* block = nullptr;
  auto params = make_graph_engine_params();
  int sample_rate = params.max_frame_count * freq;

  // show some of the decay
  if (is_random(type) && !for_osc_osc_matrix) sample_rate /= 5;

  // note: it proves tricky to reset() the oscillators
  // before the process_default call to the osc_osc_matrix
  // therefore the oversampler pointers are *not* published
  // to the block in case of processing for graphs and
  // we use an alternate means to access the modulation
  // signal inside matrix_osc::modulate_fm
  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  engine->process_default(module_osc_osc_matrix, 0);
  for (int i = 0; i <= slot; i++)
  {
    block = engine->process(module_osc, i, [max_frame_count = params.max_frame_count, sample_rate](plugin_block& block) {
      osc_engine engine(max_frame_count, sample_rate);
      engine.reset(&block);
      cv_audio_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
      engine.process<true>(block, &modulation);
    });
    jarray<float, 2> audio = jarray<float, 2>(block->state.own_audio[0][0]);
    result.push_back(graph_data(audio, 1.0f, false, {}));
  }
  engine->process_end();

  // scale to max 1, but we still want to show the gain
  for(int o = 0; o < result.size(); o++)
  {
    float max = 0;
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < params.max_frame_count; f++)
        max = std::max(max, std::fabs(result[o].audio()[c][f]));
    for (int c = 0; c < 2; c++)
      for (int f = 0; f < params.max_frame_count; f++)
        result[o].audio()[c][f] = (result[o].audio()[c][f] / (max > 0? max: 1)) * gain;
  }

  return result;
}

static graph_data
render_osc_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  graph_engine_params params = {};
  int type = state.get_plain_at(module_osc, mapping.module_slot, param_type, 0).step();
  if(state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step() == type_off) 
    return graph_data(graph_data_type::off, {});
  auto data = render_osc_graphs(state, engine, mapping.module_slot, false)[mapping.module_slot];
  std::string partition = is_random(type)? "5 Cycles": "First Cycle";
  return graph_data(data.audio(), 1.0f, false, { partition });
}

module_topo
osc_topo(int section, gui_position const& pos)
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", true, "Oscillator", "Oscillator", "Osc", module_osc, 5),
    make_module_dsp(module_stage::voice, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info_basic("{FA702356-D73E-4438-8127-0FDD01526B7E}", "Output", 0, 1 + max_osc_unison_voices)) }),
    make_module_gui(section, pos, { { 1, 1 }, { gui_dimension::auto_size, 1 } })));
  result.info.description = "Oscillator module with sine/saw/triangle/square/DSF/Karplus-Strong/noise generators, hardsync and unison support.";

  result.minimal_initializer = init_minimal;
  result.default_initializer = init_default;
  result.graph_renderer = render_osc_graph;
  result.graph_engine_factory = make_osc_graph_engine;
  result.gui.menu_handler_factory = make_osc_routing_menu_handler;
  result.engine_factory = [](auto const&, int sr, int max_frame_count) { return std::make_unique<osc_engine>(max_frame_count, sr); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{A64046EE-82EB-4C02-8387-4B9EFF69E06A}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", param_type, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  type.gui.submenu->indices.push_back(type_basic);
  type.gui.submenu->indices.push_back(type_dsf);
  type.gui.submenu->indices.push_back(type_static);
  type.gui.submenu->add_submenu("Karplus-Strong", { type_kps1, type_kps2 });
  type.info.description = std::string("Selects the oscillator algorithm. ") + 
    "Only Basic and DSF can be used as an FM target, react to oversampling, and are capable of hard-sync. " + 
    "KPS1 is regular Karplus-Strong, KPS2 is a modified version which auto-adjusts feedback according to pitch.";
  auto& note = result.params.emplace_back(make_param(
    make_topo_info_basic("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", param_note, 1), 
    make_param_dsp_voice(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  note.gui.submenu = make_midi_note_submenu();
  note.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_pitch(vs[0]); });
  note.info.description = "Oscillator base pitch. Also reacts to Voice-In base pitch.";
  auto& cent = result.params.emplace_back(make_param(
    make_topo_info_basic("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 }, 
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::near))));
  cent.info.description = "Oscillator cents, also reacts to Voice-In cents.";
  cent.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_pitch(vs[0]); });
  auto& pitch = result.params.emplace_back(make_param(
    make_topo_info_basic("{6E9030AF-EC7A-4473-B194-5DA200E7F90C}", "Pitch", param_pitch, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-128, 128, 0, 0, ""),
    make_param_gui_none()));
  pitch.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_pitch(vs[0]); });
  pitch.info.description = "Absolute pitch modulation target, also reacts to Voice-in pitch modulation.";
  auto& pb = result.params.emplace_back(make_param(
    make_topo_info("{D310300A-A143-4866-8356-F82329A76BAE}", true, "Pitch Bend", "PB", "PB", param_pb, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_none()));
  pb.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_pitch(vs[0]); });
  pb.info.description = "Pitch-bend modulation target. Also reacts to Voice-in PB modulation and master pitchbend range.";

  auto& basic = result.sections.emplace_back(make_param_section(section_basic,
    make_topo_tag_basic("{8E776EAB-DAC7-48D6-8C41-29214E338693}", "Basic"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 
      gui_dimension::auto_size, 1, gui_dimension::auto_size, 1, 
      gui_dimension::auto_size, 1, gui_dimension::auto_size, 1, gui_dimension::auto_size }))));
  basic.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  basic.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || vs[0] == type_basic; });
  auto& basic_sin_on = result.params.emplace_back(make_param(
    make_topo_info("{BD753E3C-B84E-4185-95D1-66EA3B27C76B}", true, "Basic Sin On", "Sin", "Bsc Sin", param_basic_sin_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  basic_sin_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  basic_sin_on.info.description = "Toggle sine generator on/off.";
  auto& basic_sin_mix = result.params.emplace_back(make_param(
    make_topo_info("{60FAAC91-7F69-4804-AC8B-2C7E6F3E4238}", true, "Basic Sin Mix", "Sin", "Bsc Sin", param_basic_sin_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::hslider, { 0, 1 }, make_label_none())));
  basic_sin_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_sin_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  basic_sin_mix.info.description = "Sine generator mix amount.";
  auto& basic_saw_on = result.params.emplace_back(make_param(
    make_topo_info("{A31C1E92-E7FF-410F-8466-7AC235A95BDB}", true, "Basic Saw On", "Saw", "Bsc Saw", param_basic_saw_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  basic_saw_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  basic_saw_on.info.description = "Toggle saw generator on/off.";
  auto& basic_saw_mix = result.params.emplace_back(make_param(
    make_topo_info("{A459839C-F78E-4871-8494-6D524F00D0CE}", true, "Basic Saw Mix", "Saw", "Bsc Saw", param_basic_saw_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::hslider, { 0, 3 }, make_label_none())));
  basic_saw_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_saw_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  basic_saw_mix.info.description = "Saw generator mix amount.";
  auto& basic_tri_on = result.params.emplace_back(make_param(
    make_topo_info("{F2B92036-ED14-4D88-AFE3-B83C1AAE5E76}", true, "Basic Tri On", "Tri", "Bsc Tri", param_basic_tri_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  basic_tri_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  basic_tri_on.info.description = "Toggle triangle generator on/off.";
  auto& basic_tri_mix = result.params.emplace_back(make_param(
    make_topo_info("{88F88506-5916-4668-BD8B-5C35D01D1147}", true, "Basic Tri Mix", "Tri", "Bsc Tri", param_basic_tri_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::hslider, { 0, 5 }, make_label_none())));
  basic_tri_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_tri_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  basic_tri_mix.info.description = "Triangle generator mix amount.";
  auto& basic_sqr_on = result.params.emplace_back(make_param(
    make_topo_info("{C3AF1917-64FD-481B-9C21-3FE6F8D039C4}", true, "Basic Sqr On", "Sqr", "Bsc Sqr", param_basic_sqr_on, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_basic, gui_edit_type::toggle, { 0, 6 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  basic_sqr_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_basic; });
  basic_sqr_on.info.description = "Toggle square generator on/off.";
  auto& basic_sqr_mix = result.params.emplace_back(make_param(
    make_topo_info("{B133B0E6-23DC-4B44-AA3B-6D04649271A4}", true, "Basic Sqr Mix", "Sqr", "Bsc Sqr", param_basic_sqr_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::hslider, { 0, 7 }, make_label_none())));
  basic_sqr_mix.gui.bindings.enabled.bind_params({ param_type, param_basic_sqr_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  basic_sqr_mix.info.description = "Square generator mix amount.";
  auto& basic_sqr_pw = result.params.emplace_back(make_param(
    make_topo_info("{57A231B9-CCC7-4881-885E-3244AE61107C}", true, "Basic Sqr PW", "PW", "Bsc PW", param_basic_sqr_pw, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui_single(section_basic, gui_edit_type::knob, { 0, 8 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  basic_sqr_pw.gui.bindings.enabled.bind_params({ param_type, param_basic_sqr_on }, [](auto const& vs) { return vs[0] == type_basic && vs[1] != 0; });
  basic_sqr_pw.info.description = "Square generator pulse width.";

  auto& dsf = result.sections.emplace_back(make_param_section(section_dsf,
    make_topo_tag_basic("{F6B06CEA-AF28-4AE2-943E-6225510109A3}", "DSF"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 1, 1, 1 }))));
  dsf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  dsf.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  auto& dsf_partials = result.params.emplace_back(make_param(
    make_topo_info("{21BC6524-9FDB-4551-9D3D-B180AB93B5CE}", true, "DSF Partials", "Partials", "DSF Parts", param_dsf_parts, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_log(1, 1000, 2, 20, 0, ""),
    make_param_gui_single(section_dsf, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  dsf_partials.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  dsf_partials.info.description = "Controls the number of partials (overtones).";
  auto& dsf_dist = result.params.emplace_back(make_param(
    make_topo_info("{E5E66BBD-DCC9-4A7E-AB09-2D7107548090}", true, "DSF Distance", "Distance", "DSF Dist", param_dsf_dist, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_linear(0.05, 20, 1, 2, ""),
    make_param_gui_single(section_dsf, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  dsf_dist.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  dsf_dist.info.description = "Controls the frequency distance between the base frequency and subsequent partials.";
  auto& dsf_dcy = result.params.emplace_back(make_param(
    make_topo_info("{2D07A6F2-F4D3-4094-B1C0-453FDF434CC8}", true, "DSF Decay", "Decay", "DSF Dcy", param_dsf_dcy, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dsf, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  dsf_dcy.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  dsf_dcy.info.description = "Controls the amplitude decay of successive partials.";

  auto& random = result.sections.emplace_back(make_param_section(section_rand,
    make_topo_tag_basic("{AB9E6684-243D-4579-A0AF-5BEF2C72EBA6}", "Random"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size,
      gui_dimension::auto_size, 1 }))));
  random.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& random_svf = result.params.emplace_back(make_param(
    make_topo_info("{7E47ACD4-88AC-4D3B-86B1-05CCDFB4BC7D}", true, "Rnd Filter Mode", "Flt", "Rnd.Flt", param_rand_svf, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(random_svf_items(), ""),
    make_param_gui_single(section_rand, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  random_svf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random_svf.info.description = "Continuous filter type for static noise or initial-excite filter type for Karplus-Strong.";
  auto& random_freq = result.params.emplace_back(make_param(
    make_topo_info("{289B4EA4-4A0E-4D33-98BA-7DF475B342E9}", true, "Rnd Filter Freq", "Frq", "Rnd Freq", param_rand_freq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(20, 20000, 20000, 1000, 0, "Hz"),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  random_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random_freq.info.description = std::string("Continuous filter frequency for static noise or initial-excite filter frequency for Karplus-Strong. ") + 
    "Modulation takes place only at voice start.";
  auto& random_res = result.params.emplace_back(make_param(
    make_topo_info("{3E68ACDC-9800-4A4B-9BB6-984C5A7F624B}", true, "Rnd Filter Res", "Res", "Rnd Res", param_rand_res, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  random_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random_res.info.description = std::string("Continuous filter resonance for static noise or initial-excite filter resonance for Karplus-Strong. ") + 
    "Modulation takes place only at voice start.";
  auto& random_seed = result.params.emplace_back(make_param(
    make_topo_info("{81873698-DEA9-4541-8E99-FEA21EAA2FEF}", true, "Rnd Seed", "Sed", "Rnd Seed", param_rand_seed, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  random_seed.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random_seed.info.description = "On-voice-init random seed for static noise and initial-excite stage of Karplus-Strong.";
  auto& random_step = result.params.emplace_back(make_param(
    make_topo_info("{41E7954F-27B0-48A8-932F-ACB3B3F310A7}", true, "Rnd Rate", "Rte", "Rnd Rate", param_rand_rate, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(1, 100, 10, 10, 1, "%"),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  random_step.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random_step.info.description = std::string("On-voice-init step count for static noise and initial-excite stage of Karplus-Strong. ") +
    "Modulation takes place only at voice start.";
  auto& kps_fdbk = result.params.emplace_back(make_param(
    make_topo_info("{E1907E30-9C17-42C4-B8B6-F625A388C257}", true, "K+S Feedback", "Fdbk", "KPS Fdbk", param_kps_fdbk, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 5 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  kps_fdbk.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_kps(vs[0]); });
  kps_fdbk.info.description = "Use to shorten low-frequency notes.";
  auto& kps_stretch = result.params.emplace_back(make_param( 
    make_topo_info("{9EC580EA-33C6-48E4-8C7E-300DAD341F57}", true, "K+S Stretch", "Strt", "KPS Strt", param_kps_stretch, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 6 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  kps_stretch.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_kps(vs[0]); });
  kps_stretch.info.description = "Use to stretch high-frequency notes.";
  auto& kps_mid = result.params.emplace_back(make_param(
    make_topo_info("{AE914D18-C5AB-4ABB-A43B-C80E24868F78}", true, "K+S Midpoint", "Mid", "KPS Mid", param_kps_mid, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, 127, midi_middle_c, 0),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 7 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  kps_mid.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_kps2; });
  kps_mid.info.description = std::string("In Karplus-Strong2 mode, controls the midpoint MIDI note (C4=60). ") +
    "Lower notes will be stretched less, higher notes will be stretched more. " + 
    "This tries to keep audible note lengths relatively equal.";
  
  result.sections.emplace_back(make_param_section(section_sync_gain,
    make_topo_tag_basic("{D5A040EE-5F64-4771-8581-CDC5C0CC11A8}", "Sync+Gain"),
    make_param_section_gui({ 1, 0, 1, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size }))));
  auto& gain = result.params.emplace_back(make_param(
    make_topo_info_basic("{F4224036-9246-4D90-BD0F-5867FF318D1C}", "Gain", param_gain, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1.0, 0, true),
    make_param_gui_single(section_sync_gain, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  gain.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  gain.info.description = std::string("Per-osc gain control. The same result may be had through the audio routing matrices, ") +
    "but it's just easier to work with a dedicated parameter. In particular, this control is very handy when applying an envelope to it "
    "when the oscillator is routed through a distortion module.";
  auto& sync_on = result.params.emplace_back(make_param(
    make_topo_info("{900958A4-74BC-4912-976E-45E66D4F00C7}", true, "Hard Sync On", "Hard Sync", "Hard Sync", param_hard_sync, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_sync_gain, gui_edit_type::toggle, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  sync_on.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_phase(vs[0]); });
  sync_on.info.description = "Enables hard-sync against an internal reference oscillator.";
  auto& sync_semi = result.params.emplace_back(make_param(
    make_topo_info("{FBD5ADB5-63E2-42E0-BF90-71B694E6F52C}", true, "Hard Sync Semis", "Semi", "HS Semi", param_hard_sync_semis, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0, 48, 0, 2, "Semi"),
    make_param_gui_single(section_sync_gain, gui_edit_type::knob, { 0, 2 }, make_label_none())));
  sync_semi.gui.bindings.enabled.bind_params({ param_type, param_hard_sync }, [](auto const& vs) { return can_do_phase(vs[0]) && vs[1]; });
  sync_semi.info.description = "Pitch offset of the actual oscillator against the reference oscillator.";
  auto& sync_xover = result.params.emplace_back(make_param(
    make_topo_info("{FE055A0E-4619-438B-9129-24E56437A54E}", true, "Hard Sync XOver Time", "XOver", "HS XOver", param_hard_sync_xover, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_linear(0, 5, 2.5, 2, "Ms"),
    make_param_gui_single(section_sync_gain, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  sync_xover.gui.bindings.enabled.bind_params({ param_type, param_hard_sync }, [](auto const& vs) { return can_do_phase(vs[0]) && vs[1]; });
  sync_xover.info.description = "Controls cross-over time between the synced and unsyced signal after a phase reset occurs.";

  result.sections.emplace_back(make_param_section(section_uni,
    make_topo_tag_basic("{D91778EE-63D7-4346-B857-64B2D64D0441}", "Unison"),
    make_param_section_gui({ 1, 1, 1, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1, 1, 1 }))));
  auto& uni_voices = result.params.emplace_back(make_param(
    make_topo_info("{376DE9EF-1CC4-49A0-8CA7-9CF20D33F4D8}", true, "Unison Voices", "Unison", "Unison", param_uni_voices, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, max_osc_unison_voices, 1, 0),
    make_param_gui_single(section_uni, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  uni_voices.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return vs[0] != type_off; });
  uni_voices.info.description = "Unison voice count. Oversampling, hard-sync, AM and FM are applied per-unison-voice.";
  auto& uni_phase = result.params.emplace_back(make_param(
    make_topo_info("{8F1098B6-64F9-407E-A8A3-8C3637D59A26}", true, "Unison Phase", "Phase", "Uni Phs", param_uni_phase, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_uni, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  uni_phase.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return can_do_phase(vs[0]) && vs[1] > 1; });
  uni_phase.info.description = "Phase offset for subsequent voices, to get that unison effect 'right from the start'. Only applicable to Basic and DSF generators.";
  auto& uni_dtn = result.params.emplace_back(make_param(
    make_topo_info("{FDAE1E98-B236-4B2B-8124-0B8E1EF72367}", true, "Unison Detune", "Detune", "Uni Dtn", param_uni_dtn, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.33, 0, true),
    make_param_gui_single(section_uni, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  uni_dtn.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return can_do_pitch(vs[0]) && vs[1] > 1; });
  uni_dtn.info.description = "Detune unison voices. Only applicable to Basic and DSF generators.";
  auto& uni_spread = result.params.emplace_back(make_param(
    make_topo_info("{537A8F3F-006B-4F99-90E4-F65D0DF2F59F}", true, "Unison Spread", "Spread", "Uni Sprd", param_uni_sprd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_uni, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  uni_spread.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return vs[0] != type_off && vs[1] > 1; });
  uni_spread.info.description = "Unison stereo spread, works on all oscillator modes.";

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

static float 
generate_dsf(float phase, float increment, float sr, float freq, int parts, float dist_parts, float decay)
{
  // -1: Fundamental is implicit. 
  int ps = parts - 1;
  float const decay_range = 0.99f;
  float const scale_factor = 0.975f;
  float dist_freq = freq * dist_parts;
  float max_parts = (sr * 0.5f - freq) / dist_freq;
  ps = std::min(ps, (int)max_parts);

  float n = static_cast<float>(ps);
  float w = decay * decay_range;
  float w_pow_np1 = std::pow(w, n + 1);
  float u = 2.0f * pi32 * phase;
  float v = 2.0f * pi32 * dist_freq * phase / freq;
  float a = w * std::sin(u + n * v) - std::sin(u + (n + 1) * v);
  float x = (w * std::sin(v - u) + std::sin(u)) + w_pow_np1 * a;
  float y = 1 + w * w - 2 * w * std::cos(v);
  float scale = (1.0f - w_pow_np1) / (1.0f - w);
  return check_bipolar(x * scale_factor / (y * scale));
}

osc_engine::
osc_engine(int max_frame_count, float sample_rate):
_oversampler(max_frame_count)
{
  float const kps_min_freq = 20.0f;
  _kps_max_length = (int)(std::ceil(sample_rate / kps_min_freq));
  for (int v = 0; v < max_osc_unison_voices; v++)
  {
    _kps_freqs[v] = 0;
    _kps_lengths[v] = -1;
    _kps_positions[v] = 0;
    _kps_lines[v] = std::vector<float>(_kps_max_length);
  }

  for (int s = 0; s <= max_oversampler_stages; s++)
    _context.oversampled_lanes_channels_ptrs[s] = _oversampler.get_upsampled_lanes_channels_ptrs(1 << s);
}

void
osc_engine::reset(plugin_block const* block)
{
  _kps_initialized = false;
  auto const& block_auto = block->state.own_block_automation;
  float uni_phase = block_auto[param_uni_phase][0].real();
  int uni_voices = block_auto[param_uni_voices][0].step();
  for (int v = 0; v < uni_voices; v++)
  {
    _static_svfs[v].clear();
    // Unison over static noise doesnt do detune, but it can stereo spread.
    _static_noises[v].reset(block_auto[param_rand_seed][0].step() + v);
    // Block below 20hz, certain param combinations generate very low frequency content
    _random_dcs[v].init(block->sample_rate, 20);
    _ref_phases[v] = (float)v / uni_voices * (uni_voices == 1 ? 0.0f : uni_phase);
    _sync_phases[v] = _ref_phases[v];
    _unsync_phases[v] = 0;
    _unsync_samples[v] = 0;
  }

  // publish the oversampler ptrs
  // note: it is important to do this during reset() rather than process()
  // because we have a circular dependency osc<>osc_fm
  // and the call order is reset_osc, reset_osc_fm, process_osc, process_osc_fm
  // luckily the fm matrix only needs the oversampler ptrs in the process() call
  *block->state.own_context = &_context;
}

// Cant be in the reset call because we need access to modulation.
void
osc_engine::init_kps(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  assert(!_kps_initialized);

  auto const& block_auto = block.state.own_block_automation;

  // Initial kps excite using static noise + res svf filter.
  double const kps_max_res = 0.99;
  int kps_svf = block_auto[param_rand_svf][0].step();
  int kps_seed = block_auto[param_rand_seed][0].step();

  // Frequency, rate and res are not continuous params but we fake it this way so they can 
  // participate in modulation. In particular velocity seems like a good mod source for freq.
  float kps_res = (*(*modulation)[module_osc][block.module_slot][param_rand_res][0])[0];
  float kps_freq_normalized = (*(*modulation)[module_osc][block.module_slot][param_rand_freq][0])[0];
  float kps_freq = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_freq, kps_freq_normalized);
  float kps_rate_normalized = (*(*modulation)[module_osc][block.module_slot][param_rand_rate][0])[0];
  float kps_rate = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_rate, kps_rate_normalized);

  // Initial white noise + filter amount.
  state_var_filter filter = {};
  static_noise static_noise_ = {};

  // below 50 hz gives barely any results
  float rate = 50 + (kps_rate * 0.01) * (block.sample_rate * 0.5f - 50);
  static_noise_.reset(kps_seed);
  static_noise_.update(block.sample_rate, rate, 1);
  double w = pi64 * kps_freq / block.sample_rate;
  switch (kps_svf)
  {
  case rand_svf_lpf: filter.init_lpf(w, kps_res * kps_max_res); break;
  case rand_svf_hpf: filter.init_hpf(w, kps_res * kps_max_res); break;
  case rand_svf_bpf: filter.init_bpf(w, kps_res * kps_max_res); break;
  case rand_svf_bsf: filter.init_bsf(w, kps_res * kps_max_res); break;
  case rand_svf_peq: filter.init_peq(w, kps_res * kps_max_res); break;
  default: assert(false); break;
  }
  for (int v = 0; v < max_osc_unison_voices; v++)
  {
    // we fix length at first call to generate_kps
    _kps_freqs[v] = 0;
    _kps_lengths[v] = -1;
    _kps_positions[v] = 0;
    for (int f = 0; f < _kps_max_length; f++)
    {
      float noise = static_noise_.next<true>(1, kps_seed);
      _kps_lines[v][f] = filter.next(0, unipolar_to_bipolar(noise));
    }
  }
}

template <int SVFType>
float osc_engine::generate_static(int voice, float sr, float freq_hz, float res, int seed, float rate_hz)
{
  _static_noises[voice].update(sr, rate_hz, 1);
  float result = unipolar_to_bipolar(_static_noises[voice].next<true>(1, seed));

  float const max_res = 0.99f;
  double w = pi64 * freq_hz / sr;
  if constexpr (SVFType == rand_svf_lpf)
    _static_svfs[voice].init_lpf(w, res * max_res);
  else if constexpr (SVFType == rand_svf_hpf)
    _static_svfs[voice].init_hpf(w, res * max_res);
  else if constexpr (SVFType == rand_svf_bpf)
    _static_svfs[voice].init_bpf(w, res * max_res);
  else if constexpr (SVFType == rand_svf_bsf)
    _static_svfs[voice].init_bsf(w, res * max_res);
  else if constexpr (SVFType == rand_svf_peq)
    _static_svfs[voice].init_peq(w, res * max_res);
  else
  {
    // cannot got a static_assert here that works on both msvc and gcc
    assert(false);
    return {};
  }

  result = _static_svfs[voice].next(0, result);
  result = _random_dcs[voice].next(0, result);
  return result;
}

template <bool AutoFdbk> float
osc_engine::generate_kps(int voice, float sr, float freq0, float fdbk0, float stretch, float mid_freq)
{
  // fix value at voice start
  // kps is not suitable for pitch modulation
  // but we still allow it so user can do on-note mod
  if (_kps_lengths[voice] == -1)
  {
    _kps_freqs[voice] = freq0;
    _kps_lengths[voice] = std::min((int)(sr / freq0), _kps_max_length);
  }

  // in this case feedback is affected by pitch
  float feedback = fdbk0;
  if constexpr (AutoFdbk)
  {
    feedback = 1 - feedback;
    float base = _kps_freqs[voice] <= mid_freq ? _kps_freqs[voice] / mid_freq * 0.5f : 0.5f + (1 - mid_freq / _kps_freqs[voice]) * 0.5f;
    feedback = std::pow(std::clamp(base, 0.0f, 1.0f), feedback);
  }
  check_unipolar(feedback);

  stretch *= 0.5f;
  float const min_feedback = 0.9f;
  int this_index = _kps_positions[voice];
  int next_index = (_kps_positions[voice] + 1) % _kps_lengths[voice];
  float result = _kps_lines[voice][this_index];
  _kps_lines[voice][this_index] = (0.5f + stretch) * _kps_lines[voice][this_index];
  _kps_lines[voice][this_index] += (0.5f - stretch) * _kps_lines[voice][next_index];
  _kps_lines[voice][this_index] *= min_feedback + feedback * (1.0f - min_feedback);
  if (++_kps_positions[voice] >= _kps_lengths[voice]) _kps_positions[voice] = 0;
  return _random_dcs[voice].next(0, result);
}

template <bool Graph> void
osc_engine::process(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  // allow custom data for graphs
  if (modulation == nullptr)
    modulation = &get_cv_audio_matrix_mixdown(block, false);

  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  if (type == type_off)
  {
    // still need to process to clear out the buffer in case we are mod source
    process_unison<Graph, false, false, false, false, false, false, false, false, false, -1>(block, modulation);
    return;
  }

  switch (type)
  {
  case type_dsf: process_dsf<Graph>(block, modulation); break;
  case type_basic: process_basic<Graph>(block, modulation); break;
  case type_static: process_static<Graph>(block, modulation); break;
  case type_kps1: process_unison<Graph, false, false, false, false, false, false, true, false, false, -1>(block, modulation); break;
  case type_kps2: process_unison<Graph, false, false, false, false, false, false, true, true, false, -1>(block, modulation); break;
  default: assert(false); break;
  }
}

template <bool Graph> void
osc_engine::process_dsf(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  if(block.state.own_block_automation[param_hard_sync][0].step())
    process_unison<Graph, false, false, false, false, true, true, false, false, false, -1>(block, modulation);
  else
    process_unison<Graph, false, false, false, false, true, false, false, false, false, -1>(block, modulation);
}

template <bool Graph> void
osc_engine::process_static(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int svf_mode = block_auto[param_rand_svf][0].step();
  switch (svf_mode)
  {
  case rand_svf_lpf: process_unison<Graph, false, false, false, false, false, false, false, false, true, rand_svf_lpf>(block, modulation); break;
  case rand_svf_hpf: process_unison<Graph, false, false, false, false, false, false, false, false, true, rand_svf_hpf>(block, modulation); break;
  case rand_svf_bpf: process_unison<Graph, false, false, false, false, false, false, false, false, true, rand_svf_bpf>(block, modulation); break;
  case rand_svf_bsf: process_unison<Graph, false, false, false, false, false, false, false, false, true, rand_svf_bsf>(block, modulation); break;
  case rand_svf_peq: process_unison<Graph, false, false, false, false, false, false, false, false, true, rand_svf_peq>(block, modulation); break;
  default: assert(false); break;
  }
}

template <bool Graph> void
osc_engine::process_basic(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sin = block_auto[param_basic_sin_on][0].step();
  if(sin) process_basic_sin<Graph, true>(block, modulation);
  else process_basic_sin<Graph, false>(block, modulation);
}

template <bool Graph, bool Sin> void
osc_engine::process_basic_sin(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool saw = block_auto[param_basic_saw_on][0].step();
  if (saw) process_basic_sin_saw<Graph, Sin, true>(block, modulation);
  else process_basic_sin_saw<Graph, Sin, false>(block, modulation);
}

template <bool Graph, bool Sin, bool Saw> void
osc_engine::process_basic_sin_saw(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool tri = block_auto[param_basic_tri_on][0].step();
  if (tri) process_basic_sin_saw_tri<Graph, Sin, Saw, true>(block, modulation);
  else process_basic_sin_saw_tri<Graph, Sin, Saw, false>(block, modulation);
}

template <bool Graph, bool Sin, bool Saw, bool Tri> void
osc_engine::process_basic_sin_saw_tri(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sqr = block_auto[param_basic_sqr_on][0].step();
  if (sqr) process_basic_sin_saw_tri_sqr<Graph, Sin, Saw, Tri, true>(block, modulation);
  else process_basic_sin_saw_tri_sqr<Graph, Sin, Saw, Tri, false>(block, modulation);
}

template <bool Graph, bool Sin, bool Saw, bool Tri, bool Sqr> void
osc_engine::process_basic_sin_saw_tri_sqr(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sync = block_auto[param_hard_sync][0].step() != 0;
  if (sync) process_unison<Graph, Sin, Saw, Tri, Sqr, false, true, false, false, false, -1>(block, modulation);
  else process_unison<Graph, Sin, Saw, Tri, Sqr, false, false, false, false, false, -1>(block, modulation);
}

template <bool Graph, bool Sin, bool Saw, bool Tri, bool Sqr, bool DSF, bool Sync, bool KPS, bool KPSAutoFdbk, bool Static, int StaticSVFType> 
void
osc_engine::process_unison(plugin_block& block, cv_audio_matrix_mixdown const* modulation)
{
  int generator_count = 0;
  if constexpr (Sin) generator_count++;
  if constexpr (Saw) generator_count++;
  if constexpr (Tri) generator_count++;
  if constexpr (Sqr) generator_count++;
  if constexpr (DSF) generator_count++;
  if constexpr (KPS) generator_count++;
  if constexpr (Static) generator_count++;

  static_assert(!KPSAutoFdbk || KPS);
  static_assert(StaticSVFType == -1 || Static);

  // need to clear all active outputs because we 
  // don't know if we are a modulation source
  // voice 0 is total
  auto const& block_auto = block.state.own_block_automation;
  bool on = block_auto[param_type][0].step() != type_off;
  int uni_voices = block_auto[param_uni_voices][0].step();
  for (int v = 0; v < uni_voices + 1; v++)
  {
    block.state.own_audio[0][v][0].fill(block.start_frame, block.end_frame, 0.0f);
    block.state.own_audio[0][v][1].fill(block.start_frame, block.end_frame, 0.0f);
  }
  
  assert(generator_count >= 0);
  if (!on || generator_count == 0) return;

  int oversmp_stages;
  int oversmp_factor;
  get_oversmp_info(block, oversmp_stages, oversmp_factor);

  int note = block_auto[param_note][0].step();
  int type = block_auto[param_type][0].step();
  (void)type;

  int dsf_parts = (int)std::round(block_auto[param_dsf_parts][0].real());
  int master_pb_range = block.state.all_block_automation[module_master_in][0][master_in_param_pb_range][0].step();

  int kps_mid_note = block_auto[param_kps_mid][0].step();
  float kps_mid_freq = pitch_to_freq(kps_mid_note);
  int rand_seed = block_auto[param_rand_seed][0].step();

  float dsf_dist = block_auto[param_dsf_dist][0].real();
  float uni_voice_apply = uni_voices == 1 ? 0.0f : 1.0f;
  float uni_voice_range = uni_voices == 1 ? 1.0f : (float)(uni_voices - 1);

  auto const& pb_curve = *(*modulation)[module_osc][block.module_slot][param_pb][0];
  auto const& cent_curve = *(*modulation)[module_osc][block.module_slot][param_cent][0];
  auto const& gain_curve = *(*modulation)[module_osc][block.module_slot][param_gain][0];
  auto const& pitch_curve = *(*modulation)[module_osc][block.module_slot][param_pitch][0];

  auto const& dsf_dcy_curve = *(*modulation)[module_osc][block.module_slot][param_dsf_dcy][0];
  auto const& kps_fdbk_curve = *(*modulation)[module_osc][block.module_slot][param_kps_fdbk][0];
  auto const& rand_rate_curve = *(*modulation)[module_osc][block.module_slot][param_rand_rate][0];
  auto const& kps_stretch_curve = *(*modulation)[module_osc][block.module_slot][param_kps_stretch][0];
  auto const& stc_res_curve = *(*modulation)[module_osc][block.module_slot][param_rand_res][0];
  auto const& stc_freq_curve = *(*modulation)[module_osc][block.module_slot][param_rand_freq][0];

  auto const& pw_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sqr_pw][0];
  auto const& sin_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sin_mix][0];
  auto const& saw_curve = *(*modulation)[module_osc][block.module_slot][param_basic_saw_mix][0];
  auto const& tri_curve = *(*modulation)[module_osc][block.module_slot][param_basic_tri_mix][0];
  auto const& sqr_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sqr_mix][0];

  auto const& uni_dtn_curve = *(*modulation)[module_osc][block.module_slot][param_uni_dtn][0];
  auto const& uni_sprd_curve = *(*modulation)[module_osc][block.module_slot][param_uni_sprd][0];
  auto const& sync_semis_curve = *(*modulation)[module_osc][block.module_slot][param_hard_sync_semis][0];
  auto const& voice_pitch_offset_curve = block.voice->all_cv[module_voice_in][0][voice_in_output_pitch_offset][0];

  // Fill the initial buffers.
  if constexpr (KPS)
    if (!_kps_initialized)
    {
      init_kps(block, modulation);
      _kps_initialized = true;
    }

  assert(can_do_phase(type) || oversmp_stages == 0);

  // dont oversample for graphs
  if constexpr (Graph)
  {
    oversmp_stages = 0;
    oversmp_factor = 1;
  }

  // note this must react to oversmp
  float sync_xover_ms = block_auto[param_hard_sync_xover][0].real();
  int sync_over_samples = (int)(sync_xover_ms * 0.001 * block.sample_rate * oversmp_factor);

  // need an osc that uses phase to do FM
  osc_osc_matrix_fm_modulator* fm_modulator = nullptr;
  jarray<float, 2> const* fm_modulator_sig = nullptr;
  if constexpr(!KPS && !Static)
  {
    fm_modulator = &get_osc_osc_matrix_fm_modulator(block);
    fm_modulator_sig = &fm_modulator->modulate_fm<Graph>(block, block.module_slot, modulation);
  }

  std::array<jarray<float, 2>*, max_osc_unison_voices + 1> lanes;
  for(int v = 0; v < uni_voices + 1; v++)
    lanes[v] = &block.state.own_audio[0][v];

  _oversampler.process(oversmp_stages, lanes, uni_voices + 1, block.start_frame, block.end_frame, false, [&](float** lanes_channels, int frame)
  {
    // oversampler is from 0 to (end_frame - start_frame) * oversmp_factor
    // all the not-oversampled stuff requires from start_frame to end_frame
    // so mind the bookkeeping
    int mod_index = block.start_frame + frame / oversmp_factor;
    float oversampled_rate = block.sample_rate * oversmp_factor;
    float base_pb = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_pb, pb_curve[mod_index]);
    float base_cent = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_cent, cent_curve[mod_index]);
    float base_pitch_auto = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_pitch, pitch_curve[mod_index]);
    float base_pitch_ref = note + base_cent + base_pitch_auto + base_pb * master_pb_range + voice_pitch_offset_curve[mod_index];
    float base_pitch_sync = base_pitch_ref;
    (void)base_pitch_sync;

    if constexpr (Sync)
    {
      base_pitch_sync += block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_hard_sync_semis, sync_semis_curve[mod_index]);
    }

    float detune_apply = uni_dtn_curve[mod_index] * uni_voice_apply * 0.5f;
    float spread_apply = uni_sprd_curve[mod_index] * uni_voice_apply * 0.5f;
    float min_pan = 0.5f - spread_apply;
    float max_pan = 0.5f + spread_apply;
    float min_pitch_ref = base_pitch_ref - detune_apply;
    float max_pitch_ref = base_pitch_ref + detune_apply;
    float min_pitch_sync = min_pitch_ref;
    float max_pitch_sync = max_pitch_ref;

    // All the casts to void are here for (at least) MSVC.
    // Inlining all the constexpr stuff into the oversampler generates warnings, which it doesnt do by its own.
    (void)min_pitch_sync;
    (void)max_pitch_sync;

    if constexpr (Sync)
    {
      min_pitch_sync = base_pitch_sync - detune_apply;
      max_pitch_sync = base_pitch_sync + detune_apply;
    }

    for (int v = 0; v < uni_voices; v++)
    {
      float synced_sample = 0;
      float pitch_ref = min_pitch_ref + (max_pitch_ref - min_pitch_ref) * v / uni_voice_range;
      float freq_ref = std::clamp(pitch_to_freq(pitch_ref), 10.0f, oversampled_rate * 0.5f);
      float inc_ref = freq_ref / oversampled_rate;
      float pitch_sync = pitch_ref;
      float freq_sync = freq_ref;
      float inc_sync = inc_ref;
      
      (void)pitch_sync;
      (void)freq_sync;
      (void)inc_sync;

      if constexpr (Sync)
      {
        pitch_sync = min_pitch_sync + (max_pitch_sync - min_pitch_sync) * v / uni_voice_range;
        freq_sync = std::clamp(pitch_to_freq(pitch_sync), 10.0f, oversampled_rate * 0.5f);
        inc_sync = freq_sync / oversampled_rate;
      }

      float pan = min_pan + (max_pan - min_pan) * v / uni_voice_range;

      // generated the synced sample
      float saw_mix = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_saw_mix, saw_curve[mod_index]);
      float sin_mix = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_sin_mix, sin_curve[mod_index]);
      float tri_mix = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_tri_mix, tri_curve[mod_index]);
      float sqr_mix = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_sqr_mix, sqr_curve[mod_index]);

      (void)saw_mix;
      (void)sin_mix;
      (void)tri_mix;
      (void)sqr_mix;

      if constexpr (!KPS && !Static)
      {
        // FM is oversampled, so frame, not mod_index!
        float phase_fm = (*fm_modulator_sig)[v + 1][frame];
        _sync_phases[v] += phase_fm / oversmp_factor;
        if (_sync_phases[v] < 0 || _sync_phases[v] >= 1) _sync_phases[v] -= std::floor(_sync_phases[v]);
        if (_sync_phases[v] == 1) _sync_phases[v] = 0; // this could be more efficient?
        assert(0 <= _sync_phases[v] && _sync_phases[v] < 1);
      }

      if constexpr (Saw) synced_sample += generate_saw(_sync_phases[v], inc_sync) * saw_mix;
      if constexpr (Sin) synced_sample += std::sin(2.0f * pi32 * _sync_phases[v]) * sin_mix;
      if constexpr (Tri) synced_sample += generate_triangle(_sync_phases[v], inc_sync) * tri_mix;
      if constexpr (Sqr) synced_sample += generate_sqr(_sync_phases[v], inc_sync, pw_curve[mod_index]) * sqr_mix;
      if constexpr (DSF) synced_sample = generate_dsf(_sync_phases[v], inc_sync, oversampled_rate, freq_sync, dsf_parts, dsf_dist, dsf_dcy_curve[mod_index]);

      // generate the unsynced sample and crossover
      float unsynced_sample = 0;
      (void)unsynced_sample;
      if constexpr (Sync)
      {
        if (_unsync_samples[v] > 0)
        {
          // FM is oversampled, so frame, not mod_index!
          float phase_fm = (*fm_modulator_sig)[v + 1][frame];
          _unsync_phases[v] += phase_fm / oversmp_factor;
          if (_unsync_phases[v] < 0 || _unsync_phases[v] >= 1) _unsync_phases[v] -= std::floor(_unsync_phases[v]);
          if (_unsync_phases[v] == 1) _unsync_phases[v] = 0; // this could be more efficient?
          assert(0 <= _unsync_phases[v] && _unsync_phases[v] < 1);

          if constexpr (Saw) unsynced_sample += generate_saw(_unsync_phases[v], inc_sync) * saw_mix;
          if constexpr (Sin) unsynced_sample += std::sin(2.0f * pi32 * _unsync_phases[v]) * sin_mix;
          if constexpr (Tri) unsynced_sample += generate_triangle(_unsync_phases[v], inc_sync) * tri_mix;
          if constexpr (Sqr) unsynced_sample += generate_sqr(_unsync_phases[v], inc_sync, pw_curve[mod_index]) * sqr_mix;
          if constexpr (DSF) unsynced_sample = generate_dsf(_unsync_phases[v], inc_sync, oversampled_rate, freq_sync, dsf_parts, dsf_dist, dsf_dcy_curve[mod_index]);

          increment_and_wrap_phase(_unsync_phases[v], inc_sync);
          float unsynced_weight = _unsync_samples[v]-- / (sync_over_samples + 1.0f);
          synced_sample = unsynced_weight * unsynced_sample + (1.0f - unsynced_weight) * synced_sample;
        }
      }

      if constexpr (KPS) synced_sample = generate_kps<KPSAutoFdbk>(v, oversampled_rate, freq_sync, kps_fdbk_curve[mod_index], kps_stretch_curve[mod_index], kps_mid_freq);

      if constexpr (Static)
      {
        float rand_freq_hz = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_freq, stc_freq_curve[mod_index]);
        float rand_rate_pct = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_rate, rand_rate_curve[mod_index]);
        float rand_rate_hz = rand_rate_pct * 0.01 * oversampled_rate;
        synced_sample = generate_static<StaticSVFType>(v, oversampled_rate, rand_freq_hz, stc_res_curve[mod_index], rand_seed, rand_rate_hz);
      }

      increment_and_wrap_phase(_sync_phases[v], inc_sync);

      // reset to ref phase
      if constexpr (Sync)
      {
        if(increment_and_wrap_phase(_ref_phases[v], inc_ref))
        {
          _unsync_phases[v] = _sync_phases[v];
          _unsync_samples[v] = sync_over_samples;
          _sync_phases[v] = _ref_phases[v] * inc_sync / inc_ref;
        }
      }

      lanes_channels[(v + 1) * 2 + 0][frame] = mono_pan_sqrt(0, pan) * synced_sample;
      lanes_channels[(v + 1) * 2 + 1][frame] = mono_pan_sqrt(1, pan) * synced_sample;
    }
  });
  
  // note AM is *NOT* oversampled like FM
  // now we have all the individual unison voice outputs, start modulating
  // apply AM/RM afterwards (since we can self-modulate, so modulator takes *our* own_audio into account)
  auto& am_modulator = get_osc_osc_matrix_am_modulator(block);
  auto const& am_modulated = am_modulator.modulate_am(block, block.module_slot, modulation);
  for(int v = 0; v < uni_voices; v++)
    for (int c = 0; c < 2; c++)
      for (int f = block.start_frame; f < block.end_frame; f++)
        block.state.own_audio[0][v + 1][c][f] = am_modulated[v + 1][c][f];
  
  // This means we can exceed [-1, 1] but just dividing
  // by gen_count * uni_voices gets quiet real quick.
  float attn = std::sqrt(generator_count * uni_voices);
  for (int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      float uni_total = 0;
      for (int v = 0; v < uni_voices; v++)
        uni_total += block.state.own_audio[0][v + 1][c][f];
      block.state.own_audio[0][0][c][f] = uni_total * gain_curve[f] / attn;
    }
}

}
