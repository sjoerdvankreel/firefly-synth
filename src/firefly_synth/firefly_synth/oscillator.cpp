#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/svf.hpp>
#include <firefly_synth/synth.hpp>
#include <firefly_synth/waves.hpp>
#include <firefly_synth/noise_static.hpp>

#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

enum { kps_svf_lpf, kps_svf_hpf, kps_svf_bpf, kps_svf_peq };
enum { type_off, type_basic, type_dsf, type_kps1, type_kps2, type_static };
enum { section_main, section_basic, section_dsf, section_rand, section_uni };
enum {
  param_type, param_note, param_cent, param_pitch, param_pb,
  param_basic_sin_on, param_basic_sin_mix, param_basic_saw_on, param_basic_saw_mix,
  param_basic_tri_on, param_basic_tri_mix, param_basic_sqr_on, param_basic_sqr_mix, param_basic_sqr_pwm,
  param_dsf_parts, param_dsf_dist, param_dsf_dcy,
  param_rand_svf, param_rand_freq, param_rand_res, param_rand_seed, param_rand_rate, // shared k+s/noise
  param_kps_fdbk, param_kps_stretch, param_kps_mid,
  param_uni_voices, param_uni_phase, param_uni_dtn, param_uni_sprd };

extern int const master_in_param_pb_range;
extern int const voice_in_output_pitch_offset;
// mod matrix needs this
extern int const osc_param_uni_voices = param_uni_voices;

static bool constexpr is_kps(int type) 
{ return type == type_kps1 || type == type_kps2; }
static bool constexpr is_random(int type)
{ return type == type_static || is_kps(type); }
static bool can_do_unison_phase(int type)
{ return type == type_basic || type == type_dsf; }
static bool can_do_unison(int type)
{ return type == type_basic || type == type_dsf || is_kps(type); }

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
  result.emplace_back("{C4025CB0-5B1A-4B5D-A293-CAD380F264FA}", "PEQ");
  return result;
}

// https://www.verklagekasper.de/synths/dsfsynthesis/dsfsynthesis.html
// https://blog.demofox.org/2016/06/16/synthesizing-a-pluked-string-sound-with-the-karplus-strong-algorithm/
// https://github.com/marcociccone/EKS-string-generator/blob/master/Extended%20Karplus%20Strong%20Algorithm.ipynb
class osc_engine:
public module_engine {

  // basic and dsf
  float _phase[max_unison_voices];

  // static
  static_noise _static_noise = {};

  // kps
  int _kps_max_length = {};
  bool _kps_initialized = false;
  plugin_base::dc_filter _kps_dc = {};
  std::array<int, max_unison_voices> _kps_freqs = {};
  std::array<int, max_unison_voices> _kps_lengths = {};
  std::array<int, max_unison_voices> _kps_positions = {};
  std::array<std::vector<float>, max_unison_voices> _kps_lines = {};

public:
  osc_engine(float sample_rate);
  PB_PREVENT_ACCIDENTAL_COPY(osc_engine);
  void reset(plugin_block const*) override;
  void process(plugin_block& block) override { process(block, nullptr); }
  void process(plugin_block& block, cv_matrix_mixdown const* modulation);

private:

  void init_kps(plugin_block& block, cv_matrix_mixdown const* modulation);
  float generate_kps(int voice, float sr, float freq, float fdbk, float stretch);

  void process_basic(plugin_block& block, cv_matrix_mixdown const* modulation);

  template <bool Sin> void 
  process_unison_sin(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin, bool Saw> 
  void process_unison_sin_saw(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin, bool Saw, bool Tri> 
  void process_unison_sin_saw_tri(plugin_block& block, cv_matrix_mixdown const* modulation);
  template <bool Sin, bool Saw, bool Tri, bool Sqr, bool DSF, bool KPS, bool KPSAutoFdbk, bool Static>
  void process_unison(plugin_block& block, cv_matrix_mixdown const* modulation);
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
  auto am_params = make_audio_routing_am_params(state);
  auto cv_params = make_audio_routing_cv_params(state, false);
  auto audio_params = make_audio_routing_audio_params(state, false);
  return std::make_unique<audio_routing_menu_handler>(state, cv_params, std::vector({ audio_params, am_params }));
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
render_osc_graphs(plugin_state const& state, graph_engine* engine, int slot, bool for_am_matrix)
{
  std::vector<graph_data> result;
  int note = state.get_plain_at(module_osc, slot, param_note, 0).step();
  int type = state.get_plain_at(module_osc, slot, param_type, 0).step();
  float cent = state.get_plain_at(module_osc, slot, param_cent, 0).real();
  float freq = pitch_to_freq(note + cent);
  
  plugin_block const* block = nullptr;
  auto params = make_graph_engine_params();
  int sample_rate = params.max_frame_count * freq;

  // show some of the decay
  if (is_kps(type) && !for_am_matrix) sample_rate /= 5;

  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  engine->process_default(module_am_matrix, 0);
  for (int i = 0; i <= slot; i++)
  {
    block = engine->process(module_osc, i, [sample_rate](plugin_block& block) {
      osc_engine engine(sample_rate);
      engine.reset(&block);
      cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
      engine.process(block, &modulation);
    });
    jarray<float, 2> audio = jarray<float, 2>(block->state.own_audio[0][0]);
    result.push_back(graph_data(audio, 1.0f, {}));
  }
  engine->process_end();

  // scale to 1
  for(int o = 0; o < result.size(); o++)
  {
    float max = 0;
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < params.max_frame_count; f++)
        max = std::max(max, std::fabs(result[o].audio()[c][f]));
    if(max == 0) max = 1;
    for (int c = 0; c < 2; c++)
      for (int f = 0; f < params.max_frame_count; f++)
        result[o].audio()[c][f] /= max;
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
  std::string partition = is_kps(type)? "5 Cycles": "First Cycle";
  return graph_data(data.audio(), 1.0f, { partition });
}

module_topo
osc_topo(int section, gui_colors const& colors, gui_position const& pos)
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Oscillator", "Osc", true, true, module_osc, 4),
    make_module_dsp(module_stage::voice, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{FA702356-D73E-4438-8127-0FDD01526B7E}", "Output", 0, 1 + max_unison_voices)) }),
    make_module_gui(section, colors, pos, { { 1, 1 }, { gui_dimension::auto_size, 1 } })));

  result.minimal_initializer = init_minimal;
  result.default_initializer = init_default;
  result.graph_renderer = render_osc_graph;
  result.graph_engine_factory = make_osc_graph_engine;
  result.gui.menu_handler_factory = make_osc_routing_menu_handler;
  result.engine_factory = [](auto const&, int sr, int) { return std::make_unique<osc_engine>(sr); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A64046EE-82EB-4C02-8387-4B9EFF69E06A}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", param_type, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  type.gui.submenu->indices.push_back(type_basic);
  type.gui.submenu->indices.push_back(type_dsf);
  type.gui.submenu->indices.push_back(type_static);
  type.gui.submenu->add_submenu("Karplus-Strong", { type_kps1, type_kps2 });
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
  auto& pitch = result.params.emplace_back(make_param(
    make_topo_info("{6E9030AF-EC7A-4473-B194-5DA200E7F90C}", "Pitch", param_pitch, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-128, 128, 0, 0, ""),
    make_param_gui_none()));
  pitch.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  auto& pb = result.params.emplace_back(make_param(
    make_topo_info("{D310300A-A143-4866-8356-F82329A76BAE}", "PB", param_pb, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_none()));
  pb.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

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

  auto& dsf = result.sections.emplace_back(make_param_section(section_dsf,
    make_topo_tag("{F6B06CEA-AF28-4AE2-943E-6225510109A3}", "DSF"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 1, 1, 1 }))));
  dsf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  dsf.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  auto& dsf_partials = result.params.emplace_back(make_param(
    make_topo_info("{21BC6524-9FDB-4551-9D3D-B180AB93B5CE}", "DSF.Parts", "Parts", true, false, param_dsf_parts, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_log(1, 1000, 2, 20, 0, ""),
    make_param_gui_single(section_dsf, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dsf_partials.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  auto& dsf_dist = result.params.emplace_back(make_param(
    make_topo_info("{E5E66BBD-DCC9-4A7E-AB09-2D7107548090}", "DSF.Dist", "Dist", true, false, param_dsf_dist, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_linear(0.05, 20, 1, 2, ""),
    make_param_gui_single(section_dsf, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dsf_dist.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });
  auto& dsf_dcy = result.params.emplace_back(make_param(
    make_topo_info("{2D07A6F2-F4D3-4094-B1C0-453FDF434CC8}", "DSF.Dcy", "Decay", true, false, param_dsf_dcy, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dsf, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dsf_dcy.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf; });

  auto& random = result.sections.emplace_back(make_param_section(section_rand,
    make_topo_tag("{AB9E6684-243D-4579-A0AF-5BEF2C72EBA6}", "Random"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size,
      gui_dimension::auto_size, 1 }))));
  random.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  random.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& random_svf = result.params.emplace_back(make_param(
    make_topo_info("{7E47ACD4-88AC-4D3B-86B1-05CCDFB4BC7D}", "Rnd.SVF", "SVF", true, false, param_rand_svf, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_item(random_svf_items(), ""),
    make_param_gui_single(section_rand, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  random_svf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& random_freq = result.params.emplace_back(make_param(
    make_topo_info("{289B4EA4-4A0E-4D33-98BA-7DF475B342E9}", "Rnd.Freq", "Frq", true, false, param_rand_freq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(20, 20000, 20000, 1000, 0, "Hz"),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  random_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& random_res = result.params.emplace_back(make_param(
    make_topo_info("{3E68ACDC-9800-4A4B-9BB6-984C5A7F624B}", "Rnd.Res", "Res", true, false, param_rand_res, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  random_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& random_seed = result.params.emplace_back(make_param(
    make_topo_info("{81873698-DEA9-4541-8E99-FEA21EAA2FEF}", "Rnd.Seed", "Sd", true, false, param_rand_seed, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  random_seed.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& random_step = result.params.emplace_back(make_param(
    make_topo_info("{41E7954F-27B0-48A8-932F-ACB3B3F310A7}", "Rnd.Rate", "Rt", true, false, param_rand_rate, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(1, 100, 10, 10, 1, "%"),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  random_step.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });
  auto& kps_fdbk = result.params.emplace_back(make_param(
    make_topo_info("{E1907E30-9C17-42C4-B8B6-F625A388C257}", "KPS.Fdbk", "Fd", true, false, param_kps_fdbk, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 5 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  kps_fdbk.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_kps(vs[0]); });
  auto& kps_stretch = result.params.emplace_back(make_param(
    make_topo_info("{9EC580EA-33C6-48E4-8C7E-300DAD341F57}", "KPS.Str", "St", true, false, param_kps_stretch, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 6 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  kps_stretch.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_kps(vs[0]); });
  auto& kps_mid = result.params.emplace_back(make_param(
    make_topo_info("{AE914D18-C5AB-4ABB-A43B-C80E24868F78}", "KPS.Mid", "Md", true, false, param_kps_mid, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, 127, midi_middle_c, 0),
    make_param_gui_single(section_rand, gui_edit_type::knob, { 0, 7 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  kps_mid.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_kps2; });
  
  auto& unison = result.sections.emplace_back(make_param_section(section_uni,
    make_topo_tag("{D91778EE-63D7-4346-B857-64B2D64D0441}", "Unison"),
    make_param_section_gui({ 1, 0, 1, 2 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, 1, 1 }))));
  unison.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_unison(vs[0]); });
  auto& uni_voices = result.params.emplace_back(make_param(
    make_topo_info("{376DE9EF-1CC4-49A0-8CA7-9CF20D33F4D8}", "Uni.Voices", "Unison", true, false, param_uni_voices, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_step(1, max_unison_voices, 1, 0),
    make_param_gui_single(section_uni, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  uni_voices.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return can_do_unison(vs[0]); });
  auto& uni_phase = result.params.emplace_back(make_param(
    make_topo_info("{8F1098B6-64F9-407E-A8A3-8C3637D59A26}", "Uni.Phs", "Phs", true, false, param_uni_phase, 1),
    make_param_dsp_voice(param_automate::automate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_uni, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  uni_phase.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return can_do_unison_phase(vs[0]) && vs[1] > 1; });
  auto& uni_dtn = result.params.emplace_back(make_param(
    make_topo_info("{FDAE1E98-B236-4B2B-8124-0B8E1EF72367}", "Uni.Dtn", "Dtn", true, false, param_uni_dtn, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.33, 0, true),
    make_param_gui_single(section_uni, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  uni_dtn.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return can_do_unison(vs[0]) && vs[1] > 1; });
  auto& uni_spread = result.params.emplace_back(make_param(
    make_topo_info("{537A8F3F-006B-4F99-90E4-F65D0DF2F59F}", "Uni.Sprd", "Sprd", true, false, param_uni_sprd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_uni, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  uni_spread.gui.bindings.enabled.bind_params({ param_type, param_uni_voices }, [](auto const& vs) { return can_do_unison(vs[0]) && vs[1] > 1; });

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
osc_engine(float sample_rate)
{
  float const kps_min_freq = 20.0f;
  _kps_max_length = (int)(std::ceil(sample_rate / kps_min_freq));
  for (int v = 0; v < max_unison_voices; v++)
  {
    _kps_freqs[v] = 0;
    _kps_lengths[v] = -1;
    _kps_positions[v] = 0;
    _kps_lines[v] = std::vector<float>(_kps_max_length);
  }
}

void
osc_engine::reset(plugin_block const* block)
{
  _kps_initialized = false;
  auto const& block_auto = block->state.own_block_automation;
  _static_noise.reset(block_auto[param_rand_seed][0].step());
  float uni_phase = block_auto[param_uni_phase][0].real();
  int uni_voices = block_auto[param_uni_voices][0].step();
  for (int v = 0; v < uni_voices; v++)
    _phase[v] = (float)v / uni_voices * (uni_voices == 1 ? 0.0f : uni_phase);
}

// Cant be in the reset call because we need access to modulation.
void
osc_engine::init_kps(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  assert(!_kps_initialized);

  auto const& block_auto = block.state.own_block_automation;

  // Initial kps excite using static noise + res svf filter.
  double const kps_max_res = 0.99;
  int kps_svf = block_auto[param_rand_svf][0].step();
  int kps_seed = block_auto[param_rand_seed][0].step();
  float kps_res = block_auto[param_rand_res][0].real();

  // Frequency and rate are not continuous params but we fake it this way so they can 
  // participate in modulation. In particular velocity seems like a good mod source for freq.
  float kps_freq_normalized = (*(*modulation)[module_osc][block.module_slot][param_rand_freq][0])[0];
  float kps_freq = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_freq, kps_freq_normalized);
  float kps_rate_normalized = (*(*modulation)[module_osc][block.module_slot][param_rand_rate][0])[0];
  float kps_rate = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_rate, kps_rate_normalized);

  // Block below 20hz, certain param combinations generate very low frequency content
  _kps_dc.init(block.sample_rate, 20);

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
  case kps_svf_lpf: filter.init_lpf(w, kps_res * kps_max_res); break;
  case kps_svf_hpf: filter.init_hpf(w, kps_res * kps_max_res); break;
  case kps_svf_bpf: filter.init_bpf(w, kps_res * kps_max_res); break;
  case kps_svf_peq: filter.init_peq(w, kps_res * kps_max_res); break;
  default: assert(false); break;
  }
  for (int v = 0; v < max_unison_voices; v++)
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

float
osc_engine::generate_kps(int voice, float sr, float freq, float fdbk, float stretch)
{
  float const min_feedback = 0.9f;
  stretch *= 0.5f;

  // fix value at voice start
  // kps is not suitable for pitch modulation
  // but we still allow it so user can do on-note mod
  if(_kps_lengths[voice] == -1) 
    _kps_lengths[voice] = std::min((int)(sr / freq), _kps_max_length);

  int this_index = _kps_positions[voice];
  int next_index = (_kps_positions[voice] + 1) % _kps_lengths[voice];
  float result = _kps_lines[voice][this_index];
  _kps_lines[voice][this_index] = (0.5f + stretch) * _kps_lines[voice][this_index];
  _kps_lines[voice][this_index] += (0.5f - stretch) * _kps_lines[voice][next_index];
  _kps_lines[voice][this_index] *= min_feedback + fdbk * (1.0f - min_feedback);
  if (++_kps_positions[voice] >= _kps_lengths[voice]) _kps_positions[voice] = 0;
  return _kps_dc.next(0, result);
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
  case type_static: process_unison<false, false, false, false, false, false, false, true>(block, modulation); break;
  case type_dsf: process_unison<false, false, false, false, true, false, false, false>(block, modulation); break;
  case type_kps1: process_unison<false, false, false, false, false, true, false, false>(block, modulation); break;
  case type_kps2: process_unison<false, false, false, false, false, true, true, false>(block, modulation); break;
  default: assert(false); break;
  }
}

void
osc_engine::process_basic(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sin = block_auto[param_basic_sin_on][0].step();
  if(sin) process_unison_sin<true>(block, modulation);
  else process_unison_sin<false>(block, modulation);
}

template <bool Sin> void
osc_engine::process_unison_sin(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool saw = block_auto[param_basic_saw_on][0].step();
  if (saw) process_unison_sin_saw<Sin, true>(block, modulation);
  else process_unison_sin_saw<Sin, false>(block, modulation);
}

template <bool Sin, bool Saw> void
osc_engine::process_unison_sin_saw(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool tri = block_auto[param_basic_tri_on][0].step();
  if (tri) process_unison_sin_saw_tri<Sin, Saw, true>(block, modulation);
  else process_unison_sin_saw_tri<Sin, Saw, false>(block, modulation);
}

template <bool Sin, bool Saw, bool Tri> void
osc_engine::process_unison_sin_saw_tri(plugin_block& block, cv_matrix_mixdown const* modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sqr = block_auto[param_basic_sqr_on][0].step();
  if (sqr) process_unison<Sin, Saw, Tri, true, false, false, false, false>(block, modulation);
  else process_unison<Sin, Saw, Tri, false, false, false, false, false>(block, modulation);
}

template <bool Sin, bool Saw, bool Tri, bool Sqr, bool DSF, bool KPS, bool KPSAutoFdbk, bool Static> void
osc_engine::process_unison(plugin_block& block, cv_matrix_mixdown const* modulation)
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

  // need to clear all active outputs because we 
  // don't know if we are a modulation source
  // voice 0 is total
  auto const& block_auto = block.state.own_block_automation;
  int uni_voices = block_auto[param_uni_voices][0].step();
  for (int v = 0; v < uni_voices + 1; v++)
  {
    block.state.own_audio[0][v][0].fill(block.start_frame, block.end_frame, 0.0f);
    block.state.own_audio[0][v][1].fill(block.start_frame, block.end_frame, 0.0f);
  }

  if (generator_count == 0) return;

  int note = block_auto[param_note][0].step();
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
  auto const& pitch_curve = *(*modulation)[module_osc][block.module_slot][param_pitch][0];

  auto const& dsf_dcy_curve = *(*modulation)[module_osc][block.module_slot][param_dsf_dcy][0];
  auto const& kps_fdbk_curve = *(*modulation)[module_osc][block.module_slot][param_kps_fdbk][0];
  auto const& rand_rate_curve = *(*modulation)[module_osc][block.module_slot][param_rand_rate][0];
  auto const& kps_stretch_curve = *(*modulation)[module_osc][block.module_slot][param_kps_stretch][0];

  auto const& sin_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sin_mix][0];
  auto const& saw_curve = *(*modulation)[module_osc][block.module_slot][param_basic_saw_mix][0];
  auto const& tri_curve = *(*modulation)[module_osc][block.module_slot][param_basic_tri_mix][0];
  auto const& sqr_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sqr_mix][0];
  auto const& pwm_curve = *(*modulation)[module_osc][block.module_slot][param_basic_sqr_pwm][0];

  auto const& uni_dtn_curve = *(*modulation)[module_osc][block.module_slot][param_uni_dtn][0];
  auto const& uni_sprd_curve = *(*modulation)[module_osc][block.module_slot][param_uni_sprd][0];
  auto const& voice_pitch_offset_curve = block.voice->all_cv[module_voice_in][0][voice_in_output_pitch_offset][0];

  // Fill the initial buffers.
  if constexpr (KPS)
    if (!_kps_initialized)
    {
      init_kps(block, modulation);
      _kps_initialized = true;
    }

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float base_pb = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_pb, pb_curve[f]);
    float base_cent = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_cent, cent_curve[f]);
    float base_pitch_auto = block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_pitch, pitch_curve[f]);
    float base_pitch = note + base_cent + base_pitch_auto + base_pb * master_pb_range + voice_pitch_offset_curve[f];

    float detune_apply = uni_dtn_curve[f] * uni_voice_apply * 0.5f;
    float spread_apply = uni_sprd_curve[f] * uni_voice_apply * 0.5f;
    float min_pan = 0.5f - spread_apply;
    float max_pan = 0.5f + spread_apply;
    float min_pitch = base_pitch - detune_apply;
    float max_pitch = base_pitch + detune_apply;

    for (int v = 0; v < uni_voices; v++)
    {
      float sample = 0;
      float pitch = min_pitch + (max_pitch - min_pitch) * v / uni_voice_range;
      float freq = std::clamp(pitch_to_freq(pitch), 10.0f, block.sample_rate * 0.5f);
      float inc = freq / block.sample_rate;
      float pan = min_pan + (max_pan - min_pan) * v / uni_voice_range;

      if constexpr (Saw) sample += generate_saw(_phase[v], inc) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_saw_mix, saw_curve[f]);
      if constexpr (Sin) sample += std::sin(2.0f * pi32 * _phase[v]) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_sin_mix, sin_curve[f]);
      if constexpr (Tri) sample += generate_triangle(_phase[v], inc) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_tri_mix, tri_curve[f]);
      if constexpr (Sqr) sample += generate_sqr(_phase[v], inc, pwm_curve[f]) * block.normalized_to_raw_fast<domain_type::linear>(module_osc, param_basic_sqr_mix, sqr_curve[f]);
      
      if constexpr (DSF) sample = generate_dsf(_phase[v], inc, block.sample_rate, freq, dsf_parts, dsf_dist, dsf_dcy_curve[f]);

      if constexpr (Static)
      {
        float rand_rate_pct = block.normalized_to_raw_fast<domain_type::log>(module_osc, param_rand_rate, rand_rate_curve[f]);
        float rand_rate_hz = rand_rate_pct * 0.01 * block.sample_rate * 0.5;
        _static_noise.update(block.sample_rate, rand_rate_hz, 1);
        sample = unipolar_to_bipolar(_static_noise.next<true>(1, rand_seed));
      }

      if constexpr (KPS) 
      {
        if (_kps_lengths[v] == -1)
          _kps_freqs[v] = freq;
        float kps_freq = _kps_freqs[v];

        float feedback = kps_fdbk_curve[f];
        if constexpr(KPSAutoFdbk)
        {
          feedback = 1 - feedback;
          float base = kps_freq <= kps_mid_freq ? kps_freq / kps_mid_freq * 0.5f: 0.5f + (1 - kps_mid_freq / kps_freq) * 0.5f;
          feedback = std::pow(std::clamp(base, 0.0f, 1.0f), feedback);
        }
        check_unipolar(feedback);
        sample = generate_kps(v, block.sample_rate, freq, feedback, kps_stretch_curve[f]);
      }

      // kps goes a bit out of bounds
      if constexpr(!KPS)
        check_bipolar(sample / generator_count);

      increment_and_wrap_phase(_phase[v], inc);
      block.state.own_audio[0][v + 1][0][f] = mono_pan_sqrt(0, pan) * sample;
      block.state.own_audio[0][v + 1][1][f] = mono_pan_sqrt(1, pan) * sample;
    }
  }

  // now we have all the individual unison voice outputs, start modulating
  // apply AM/RM afterwards (since we can self-modulate, so modulator takes *our* own_audio into account)
  auto& modulator = get_am_matrix_modulator(block);
  auto const& modulated = modulator.modulate(block, block.module_slot, modulation);
  for(int v = 0; v < uni_voices; v++)
    for (int c = 0; c < 2; c++)
      for (int f = block.start_frame; f < block.end_frame; f++)
        block.state.own_audio[0][v + 1][c][f] = modulated[v + 1][c][f];
  
  // This means we can exceed [-1, 1] but just dividing
  // by gen_count * uni_voices gets quiet real quick.
  float attn = std::sqrt(generator_count * uni_voices);
  for (int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      float uni_total = 0;
      for (int v = 0; v < uni_voices; v++)
        uni_total += block.state.own_audio[0][v + 1][c][f];
      block.state.own_audio[0][0][c][f] = uni_total / attn;
    }
}

}
