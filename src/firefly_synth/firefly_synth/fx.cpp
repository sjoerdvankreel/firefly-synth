#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/oversampler.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <firefly_synth/waves.hpp>

#include <cmath>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth {

static double const comb_max_ms = 5;
static double const comb_min_ms = 0.1;
static double const flt_min_freq = 20;
static double const flt_max_freq = 20000;
static float const log_half = std::log(0.5f);

enum { shape_clip_clip, shape_clip_tanh };
enum { type_off, type_svf, type_comb, type_shaper, type_delay };
enum { shape_over_1, shape_over_2, shape_over_4, shape_over_8, shape_over_16 };
enum { section_type, section_svf, section_comb, section_shape, section_delay };
enum { scratch_shape_x, scratch_shape_y, scratch_shape_drive_raw, scratch_count };
enum { svf_type_lpf, svf_type_hpf, svf_type_bpf, svf_type_bsf, svf_type_apf, svf_type_peq, svf_type_bll, svf_type_lsh, svf_type_hsh };
enum { param_type, 
  param_svf_type, param_svf_freq, param_svf_res, param_svf_gain, param_svf_kbd,
  param_comb_dly_plus, param_comb_gain_plus, param_comb_dly_min, param_comb_gain_min,
  param_shape_over, param_shape_clip, param_shape_type, param_shape_x, param_shape_y, param_shape_mix, param_shape_drive, param_shape_lpf, param_shape_hpf,
  param_delay_tempo, param_delay_feedback };

static bool svf_has_gain(int svf_type) { return svf_type >= svf_type_bll; }
static bool shp_has_skew_x(multi_menu const& menu, int type) { return menu.multi_items[type].index2 != wave_skew_type_off; }
static bool shp_has_skew_y(multi_menu const& menu, int type) { return menu.multi_items[type].index3 != wave_skew_type_off; }

static std::vector<list_item>
type_items(bool global)
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  result.emplace_back("{9CB55AC0-48CB-43ED-B81E-B97C08771815}", "SVF");
  result.emplace_back("{8140F8BC-E4FD-48A1-B147-CD63E9616450}", "Comb");
  result.emplace_back("{277BDD6B-C1F8-4C33-90DB-F4E144FE06A6}", "Shape");
  if(global) result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  return result;
}

static std::vector<list_item>
svf_type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{8B7E5C75-C3F6-4F53-900C-0A75703F5570}", "LPF");
  result.emplace_back("{673C872A-F740-431C-8CD3-F577CE984C2D}", "HPF");
  result.emplace_back("{9AADCB70-4753-400A-B4C3-A68AFA60273E}", "BPF");
  result.emplace_back("{3516F76F-1162-4180-AAE6-BA658A6035C6}", "BSF");
  result.emplace_back("{57BA905C-9735-4090-9D4D-75F6CD639387}", "APF");
  result.emplace_back("{13013D87-0DBA-47D7-BE59-F6B1B65CA118}", "PEQ");
  result.emplace_back("{463BAD99-6E33-4052-B6EF-31D6D781F002}", "BLL");
  result.emplace_back("{0ECA44F9-57AD-44F4-A066-60A166F4BD86}", "LSH");
  result.emplace_back("{D28FA8B1-3D45-4C80-BAA3-C6735FA4A5E2}", "HSH");
  return result;
}

static std::vector<list_item>
shape_clip_items()
{
  std::vector<list_item> result;
  result.emplace_back("{FAE2F1EB-248D-4BA2-A008-07C2CD56EB71}", "Clip");
  result.emplace_back("{E40AE5EA-2E84-436F-960E-2D8733F0AA42}", "Tanh");
  return result;
}

static std::vector<list_item>
shape_over_items()
{
  std::vector<list_item> result;
  result.emplace_back("{AFE72C25-18F2-4DB5-A3F0-1A188032F6FB}", "1X");
  result.emplace_back("{0E961515-1089-4E65-99C0-3A493253CF07}", "2X");
  result.emplace_back("{59C0B496-3241-4D56-BE1F-D7B4B08DB64D}", "4X");
  result.emplace_back("{BAA4877E-1A4A-4D71-8B80-1AC567B7A37B}", "8X");
  result.emplace_back("{CE3CBAC6-4E7F-418E-90F5-19FEF6DF399D}", "16X");
  return result;
}

class fx_engine: 
public module_engine {  

  bool const _global;

  // svf
  double _ic1eq[2];
  double _ic2eq[2];

  // comb
  int _comb_pos = {};
  int _comb_samples = {};
  std::vector<double> _comb_in[2];
  std::vector<double> _comb_out[2];

  // delay
  int _dly_pos = {};
  int const _dly_capacity = {};
  jarray<float, 2> _dly_buffer = {};

  // shaper with fixed dc filter @20hz
  // https://www.dsprelated.com/freebooks/filters/DC_Blocker.html
  double _shp_dc_flt_r = 0;
  double _shp_dc_flt_x0[2];
  double _shp_dc_flt_y0[2];
  oversampler _shp_oversampler;
  std::vector<multi_menu_item> _shape_type_items;

  void process_comb(plugin_block& block, cv_matrix_mixdown const& modulation);
  void process_delay(plugin_block& block, cv_matrix_mixdown const& modulation);
 
  // https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
  void process_svf(plugin_block& block, cv_matrix_mixdown const& modulation);
  template <class Init> 
  void process_svf_type(plugin_block& block, cv_matrix_mixdown const& modulation, Init init);
  
  template <bool Graph>
  void process_shaper(plugin_block& block, cv_matrix_mixdown const& modulation);
  template <bool Graph, class Clip>
  void process_shaper_clip(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip);
  template <bool Graph, class Clip, class Shape>
  void process_shaper_clip_shape(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip, Shape shape);
  template <bool Graph, class Clip, class Shape, class SkewX>
  void process_shaper_clip_shape_x(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x);
  template <bool Graph, class Clip, class Shape, class SkewX, class SkewY>
  void process_shaper_clip_shape_xy(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x, SkewY skew_y);

public:
  void reset(plugin_block const*) override;
  void process(plugin_block& block) override { process<false>(block, nullptr, nullptr); }
  template<bool Graph> 
  void process(plugin_block& block, cv_matrix_mixdown const* modulation, jarray<float, 2> const* audio_in);

  PB_PREVENT_ACCIDENTAL_COPY(fx_engine);
  fx_engine(bool global, int sample_rate, int max_frame_count, std::vector<multi_menu_item> const& shape_type_items);
};

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vfx, 0, param_type, 0, "SVF");
  state.set_text_at(module_vfx, 0, param_svf_type, 0, "LPF");
  state.set_text_at(module_vfx, 0, param_svf_res, 0, "50");
  state.set_text_at(module_vfx, 0, param_svf_freq, 0, "20");
}

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_gfx, 0, param_type, 0, "SVF");
  state.set_text_at(module_gfx, 0, param_svf_type, 0, "LPF");
  state.set_text_at(module_gfx, 1, param_type, 0, "Delay");
  state.set_text_at(module_gfx, 1, param_delay_tempo, 0, "3/16");
}

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result;
  result.bpm = 120;
  result.max_frame_count = 4800;
  result.midi_key = midi_middle_c;
  return result;   
}

static std::unique_ptr<graph_engine>
make_graph_engine(plugin_desc const* desc)
{
  auto params = make_graph_engine_params();
  return std::make_unique<graph_engine>(desc, params);
}

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, int param, 
  param_topo_mapping const& mapping, std::vector<multi_menu_item> const& shape_type_items)
{
  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step();
  if(type == type_off) return graph_data(graph_data_type::off, {});

  int frame_count = -1;
  int sample_rate = -1;
  jarray<float, 2> audio_in;
  auto const params = make_graph_engine_params();
  if (type == type_shaper)
  {
    frame_count = 200;
    sample_rate = 200;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for(int i = 0; i <= 199; i++)
      audio_in[0][i] = audio_in[1][i] = unipolar_to_bipolar((float)i / 199);
  } else if(type == type_svf || type == type_comb)
  {
    frame_count = 4800;
    sample_rate = 48000;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    audio_in[0][0] = 1;
    audio_in[1][0] = 1;
  } else if (type == type_delay)
  {
    int count = 10;
    sample_rate = 50;
    frame_count = 200;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for (int i = 0; i < count; i++)
    {
      float sample = std::sin(i / (float)count * pi32 * 2.0f) * (1 - i / (float)count);
      audio_in[0][i] = sample;
      audio_in[1][i] = sample;
    }
  }

  engine->process_begin(&state, sample_rate, frame_count, -1);
  auto const* block = engine->process(
    mapping.module_index, mapping.module_slot, [mapping, sample_rate, frame_count, &audio_in, &shape_type_items](plugin_block& block) {
    bool global = mapping.module_index == module_gfx;
    fx_engine engine(global, sample_rate, frame_count, shape_type_items);
    engine.reset(&block);
    cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
    engine.process<true>(block, &modulation, &audio_in);
  });
  engine->process_end();

  auto const& audio = block->state.own_audio[0][0][0];
  if (type == type_delay || type == type_shaper)
  {
    std::string partition = type == type_delay? "IR": "Shape";
    std::vector<float> series(audio.cbegin(), audio.cbegin() + frame_count);
    return graph_data(jarray<float, 1>(series), true, { partition });
  }

  // comb / svf
  auto response = fft(audio.data());
  if (type == type_comb)
    return graph_data(jarray<float, 1>(response), false, { "24 kHz" });
  // remap over 0.8 just to look pretty
  std::vector<float> response_mapped(log_remap_series_x(response, 0.8f));
  return graph_data(jarray<float, 1>(response_mapped), false, { "24 kHz" });
}

module_topo
fx_topo(int section, gui_colors const& colors, gui_position const& pos, bool global)
{
  auto shaper_type_menu = make_wave_multi_menu(true);
  auto const voice_info = make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Voice FX", "V.FX", true, true, module_vfx, 10);
  auto const global_info = make_topo_info("{31EF3492-FE63-4A59-91DA-C2B4DD4A8891}", "Global FX", "G.FX", true, true, module_gfx, 10);
  module_stage stage = global ? module_stage::output : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::audio, scratch_count, {
      make_module_dsp_output(false, make_topo_info("{E7C21225-7ED5-45CC-9417-84A69BECA73C}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { gui_dimension::auto_size, 1 } })));
 
  result.graph_engine_factory = make_graph_engine;
  if (global) result.default_initializer = init_global_default;
  if (!global) result.default_initializer = init_voice_default;
  result.gui.menu_handler_factory = [global](plugin_state* state) {
    return make_audio_routing_menu_handler(state, global); };
  result.engine_factory = [global, shape_type_items = shaper_type_menu.multi_items](auto const&, int sample_rate, int max_frame_count) {
    return std::make_unique<fx_engine>(global, sample_rate, max_frame_count, shape_type_items); };
  result.graph_renderer = [shape_type_items = shaper_type_menu.multi_items](auto const& state, auto* engine, int param, auto const& mapping) {
      return render_graph(state, engine, param, mapping, shape_type_items); };

  result.sections.emplace_back(make_param_section(section_type,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0 }, { 1, 1 })));
  result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "Type", param_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(type_items(global), ""),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));

  auto& svf = result.sections.emplace_back(make_param_section(section_svf,
    make_topo_tag("{DFA6BD01-8F89-42CB-9D0E-E1902193DD5E}", "SVF"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { gui_dimension::auto_size, 1, 1, 1, 1 } })));
  svf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || vs[0] == type_svf; });
  auto& svf_type = result.params.emplace_back(make_param(
    make_topo_info("{784282D2-89DB-4053-9206-E11C01F37754}", "SVF.Type", "Type", true, false, param_svf_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(svf_type_items(), ""),
    make_param_gui_single(section_svf, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  svf_type.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  auto& svf_freq = result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "SVF.Frq", "Frq", true, false, param_svf_freq, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(flt_min_freq, flt_max_freq, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 1 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  auto& svf_res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "SVF.Res", "Res", true, false, param_svf_res, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  auto& svf_gain = result.params.emplace_back(make_param(
    make_topo_info("{FE108A32-770A-415B-9C85-449ABF6A944C}", "SVF.Gain", "Gain", true, false, param_svf_gain, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(-24, 24, 0, 1, "dB"),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_gain.gui.bindings.enabled.bind_params({ param_type, param_svf_type }, [](auto const& vs) { return vs[0] == type_svf && svf_has_gain(vs[1]); });
  auto& svf_kbd = result.params.emplace_back(make_param(
    make_topo_info("{9EEA6FE0-983E-4EC7-A47F-0DFD79D68BCB}", "SVF.Kbd", "Kbd", true, false, param_svf_kbd, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-2, 2, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_kbd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });

  auto& comb = result.sections.emplace_back(make_param_section(section_comb,
    make_topo_tag("{54CF060F-3EE7-4F42-921F-612F8EEA8EB0}", "Comb"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 1, 1, 1, 1 } })));
  comb.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_dly_plus = result.params.emplace_back(make_param(
    make_topo_info("{097ECBDB-1129-423C-9335-661D612A9945}", "Cmb.Dly+", "Dly+", true, false, param_comb_dly_plus, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_dly_plus.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_gain_plus = result.params.emplace_back(make_param(
    make_topo_info("{3069FB5E-7B17-4FC4-B45F-A9DFA383CAA9}", "Cmb.Gain+", "Gain+", true, false, param_comb_gain_plus, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0.5, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_gain_plus.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_dly_min = result.params.emplace_back(make_param(
    make_topo_info("{D4846933-6AED-4979-AA1C-2DD80B68404F}", "Cmb.Dly-", "Dly-", true, false, param_comb_dly_min, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_dly_min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_gain_min = result.params.emplace_back(make_param(
    make_topo_info("{9684165E-897B-4EB7-835D-D5AAF8E61E65}", "Cmb.Gain-", "Gain-", true, false, param_comb_gain_min, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_gain_min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });

  std::vector<int> shaper_col_distrib(9, gui_dimension::auto_size);
  auto& shape = result.sections.emplace_back(make_param_section(section_shape,
    make_topo_tag("{4FD908CC-0EBA-4ADD-8622-EB95013CD429}", "Shape"),
    make_param_section_gui({ 0, 1 }, { { 1 }, shaper_col_distrib })));
  shape.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });
  auto& shape_over = result.params.emplace_back(make_param(
    make_topo_info("{99C6E4A8-F90A-41DC-8AC7-4078A6DE0031}", "Shp.OverSmp", "Shp.OverSmp", true, false, param_shape_over, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(shape_over_items(), "2X"),
    make_param_gui_single(section_shape, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  shape_over.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });
  auto& shape_clip = result.params.emplace_back(make_param(
    make_topo_info("{810325E4-C3AB-48DA-A770-65887DF57845}", "Shp.Clip", "Clip", true, false, param_shape_clip, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(shape_clip_items(), "Clip"),
    make_param_gui_single(section_shape, gui_edit_type::autofit_list, { 0, 1 }, make_label_none())));
  shape_clip.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });
  auto& shape_type = result.params.emplace_back(make_param(
    make_topo_info("{BFB5A04F-5372-4259-8198-6761BA52ADEB}", "Shp.Type.SkewX/SkewY", param_shape_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(shaper_type_menu.items, "Sin.Off/Off"),
    make_param_gui_single(section_shape, gui_edit_type::autofit_list, { 0, 2 }, make_label_none())));
  shape_type.gui.submenu = shaper_type_menu.submenu;
  shape_type.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });
  auto& shape_x = result.params.emplace_back(make_param(
    make_topo_info("{94A94B06-6217-4EF5-8BA1-9F77AE54076B}", "Shp.X", "X", true, false, param_shape_x, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_x.gui.bindings.enabled.bind_params({ param_type, param_shape_type }, [shaper_type_menu](auto const& vs) { 
    return vs[0] == type_shaper && shp_has_skew_x(shaper_type_menu, vs[1]); });
  auto& shape_y = result.params.emplace_back(make_param(
    make_topo_info("{042570BF-6F02-4F91-9805-6C49FE9A3954}", "Shp.Y", "Y", true, false, param_shape_y, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_y.gui.bindings.enabled.bind_params({ param_type, param_shape_type }, [shaper_type_menu](auto const& vs) { 
    return vs[0] == type_shaper && shp_has_skew_y(shaper_type_menu, vs[1]); });
  auto& shape_mix = result.params.emplace_back(make_param(
    make_topo_info("{667D9997-5BE1-48C7-9B50-4F178E2D9FE5}", "Shp.Mix", "Mix", true, false, param_shape_mix, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 5 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; }); 
  auto& shape_drive = result.params.emplace_back(make_param(
    make_topo_info("{3FC57F28-075F-44A2-8D0D-6908447AE87C}", "Shp.Drv", "Drv", true, false, param_shape_drive, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(0.1, 32, 1, 1, 2, "%"),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 6 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_drive.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });
  auto& shp_lpf = result.params.emplace_back(make_param(
    make_topo_info("{C82BC20D-2F1E-4001-BCFB-0C8945D1B329}", "Shp.LP", "LP", true, false, param_shape_lpf, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(flt_min_freq, flt_max_freq, flt_max_freq, 1000, 0, "Hz"),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 7 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shp_lpf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });
  auto& shp_hpf = result.params.emplace_back(make_param(
    make_topo_info("{A9F6D41F-3C99-44DD-AAAA-BDC1FEEFB250}", "Shp.HP", "HP", true, false, param_shape_hpf, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(flt_min_freq, flt_max_freq, flt_min_freq, 1000, 0, "Hz"),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 8 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shp_hpf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_shaper; });

  // delay lines and reverb global only, per-voice uses too much memory
  if(!global) return result;
  auto& delay = result.sections.emplace_back(make_param_section(section_delay,
    make_topo_tag("{E92225CF-21BF-459C-8C9D-8E50285F26D4}", "Delay"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 1, 1 } })));
  delay.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_tempo = result.params.emplace_back(make_param(
    make_topo_info("{C2E282BA-9E4F-4AE6-A055-8B5456780C66}", "Dly.Tempo", "Tempo", true, false, param_delay_tempo, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_timesig_default(false, {3, 16}),
    make_param_gui_single(section_delay, gui_edit_type::list, { 0, 0 }, make_label_none())));
  delay_tempo.gui.submenu = make_timesig_submenu(delay_tempo.domain.timesigs);
  delay_tempo.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_fdbk = result.params.emplace_back(make_param(
    make_topo_info("{037E4A64-8F80-4E0A-88A0-EE1BB83C99C6}", "Dly.Fdbk", "Fdbk", true, false, param_delay_feedback, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 1 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_fdbk.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  return result;
}

static void
init_svf(
  double w, double res, double g, double a,
  double& k, double& a1, double& a2, double& a3)
{
  k = (2 - 2 * res) / a;
  a1 = 1 / (1 + g * (g + k));
  a2 = g * a1;
  a3 = g * a2;
}

static void
init_svf_lpf(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3, 
  double& m0, double& m1, double& m2)
{
  double k;
  init_svf(w, res, std::tan(w), 1, k, a1, a2, a3);
  m0 = 0; m1 = 0; m2 = 1;
}

static void
init_svf_hpf(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3, 
  double& m0, double& m1, double& m2)
{
  double k;
  init_svf(w, res, std::tan(w), 1, k, a1, a2, a3);
  m0 = 1; m1 = -k; m2 = -1;
}

static void
init_svf_bpf(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  init_svf(w, res, std::tan(w), 1, k, a1, a2, a3);
  m0 = 0; m1 = 1; m2 = 0;
}

static void
init_svf_bsf(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  init_svf(w, res, std::tan(w), 1, k, a1, a2, a3);
  m0 = 1; m1 = -k; m2 = 0;
}

static void
init_svf_apf(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  init_svf(w, res, std::tan(w), 1, k, a1, a2, a3);
  m0 = 1; m1 = -2 * k; m2 = 0;
}

static void
init_svf_peq(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  init_svf(w, res, std::tan(w), 1, k, a1, a2, a3);
  m0 = 1; m1 = -k; m2 = -2;
}

static void
init_svf_bll(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  double a = std::pow(10.0, db_gain / 40.0);
  init_svf(w, res, std::tan(w), a, k, a1, a2, a3);
  m0 = 1; m1 = k * (a * a - 1); m2 = 0;
}

static void
init_svf_lsh(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  double a = std::pow(10.0, db_gain / 40.0);
  init_svf(w, res, std::tan(w) / std::sqrt(a), 1, k, a1, a2, a3);
  m0 = 1; m1 = k * (a - 1); m2 = a * a - 1;
}

static void
init_svf_hsh(
  double w, double res, double db_gain,
  double& a1, double& a2, double& a3,
  double& m0, double& m1, double& m2)
{
  double k;
  double a = std::pow(10.0, db_gain / 40.0);
  init_svf(w, res, std::tan(w) * std::sqrt(a), 1, k, a1, a2, a3);
  m0 = a * a; m1 = k * (1 - a) * a; m2 = 1 - a * a;
}

fx_engine::
fx_engine(bool global, int sample_rate, int max_frame_count, std::vector<multi_menu_item> const& shape_type_items) :
_global(global), _dly_capacity(sample_rate * 10), 
_shp_oversampler(max_frame_count, false, false, false),
_shape_type_items(shape_type_items)
{ 
  _comb_samples = comb_max_ms * sample_rate * 0.001;
  _comb_in[0] = std::vector<double>(_comb_samples, 0.0);
  _comb_in[1] = std::vector<double>(_comb_samples, 0.0);
  _comb_out[0] = std::vector<double>(_comb_samples, 0.0);
  _comb_out[1] = std::vector<double>(_comb_samples, 0.0);
  if(global) _dly_buffer.resize(jarray<int, 1>(2, _dly_capacity));
}

void
fx_engine::reset(plugin_block const* block)
{
  _dly_pos = 0;
  _comb_pos = 0;
  _ic1eq[0] = 0;
  _ic1eq[1] = 0;
  _ic2eq[0] = 0;
  _ic2eq[1] = 0;
  _shp_dc_flt_x0[0] = 0;
  _shp_dc_flt_x0[1] = 0;
  _shp_dc_flt_y0[0] = 0;
  _shp_dc_flt_y0[1] = 0;
  _shp_dc_flt_r = 1 - (pi32 * 2 * 20 / block->sample_rate);

  auto const& block_auto = block->state.own_block_automation;
  int type = block_auto[param_type][0].step();
  if (_global && type == type_delay) _dly_buffer.fill(0);
  if (type == type_comb)
  {
    std::fill(_comb_in[0].begin(), _comb_in[0].end(), 0.0f);
    std::fill(_comb_in[1].begin(), _comb_in[1].end(), 0.0f);
    std::fill(_comb_out[0].begin(), _comb_out[0].end(), 0.0f);
    std::fill(_comb_out[1].begin(), _comb_out[1].end(), 0.0f);
  }    
}

template <bool Graph> void
fx_engine::process(plugin_block& block, 
  cv_matrix_mixdown const* modulation, jarray<float, 2> const* audio_in)
{ 
  if (audio_in == nullptr)
  {
    int this_module = _global ? module_gfx : module_vfx;
    auto& mixer = get_audio_matrix_mixer(block, _global);
    audio_in = &mixer.mix(block, this_module, block.module_slot);
  }
   
  for (int c = 0; c < 2; c++)
    (*audio_in)[c].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][c]);  
  
  int type = block.state.own_block_automation[param_type][0].step();  
  if(type == type_off) return;
  if (modulation == nullptr) modulation = &get_cv_matrix_mixdown(block, _global);  

  switch (type)
  {
  case type_svf: process_svf(block, *modulation); break;
  case type_comb: process_comb(block, *modulation); break;
  case type_delay: process_delay(block, *modulation); break;
  case type_shaper: process_shaper<Graph>(block, *modulation); break;
  default: assert(false); break;
  }
}

void
fx_engine::process_delay(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  float max_feedback = 0.9f;
  int this_module = _global ? module_gfx : module_vfx;
  float time = get_timesig_time_value(block, this_module, param_delay_tempo);
  int samples = std::min(block.sample_rate * time, (float)_dly_capacity);

  auto const& feedback_curve = *modulation[this_module][block.module_slot][param_delay_feedback][0];
  for (int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      float dry = block.state.own_audio[0][0][c][f];
      float wet = _dly_buffer[c][(_dly_pos + f + _dly_capacity - samples) % _dly_capacity];
      block.state.own_audio[0][0][c][f] = dry + wet * feedback_curve[f] * max_feedback;
      _dly_buffer[c][(_dly_pos + f) % _dly_capacity] = block.state.own_audio[0][0][c][f];
    }
  _dly_pos += block.end_frame - block.start_frame;
  _dly_pos %= _dly_capacity;
}

void
fx_engine::process_comb(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  float const feedback_factor = 0.98;
  int this_module = _global ? module_gfx : module_vfx;
  auto const& dly_min_curve = *modulation[this_module][block.module_slot][param_comb_dly_min][0];
  auto const& dly_plus_curve = *modulation[this_module][block.module_slot][param_comb_dly_plus][0];
  auto const& gain_min_curve = *modulation[this_module][block.module_slot][param_comb_gain_min][0];
  auto const& gain_plus_curve = *modulation[this_module][block.module_slot][param_comb_gain_plus][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float dly_min = block.normalized_to_raw(this_module, param_comb_dly_min, dly_min_curve[f]);
    float dly_plus = block.normalized_to_raw(this_module, param_comb_dly_plus, dly_plus_curve[f]);
    float gain_min = block.normalized_to_raw(this_module, param_comb_gain_min, gain_min_curve[f]);
    float gain_plus = block.normalized_to_raw(this_module, param_comb_gain_plus, gain_plus_curve[f]);
    float dly_min_samples_t = dly_min * block.sample_rate * 0.001;
    float dly_plus_samples_t = dly_plus * block.sample_rate * 0.001;
    float dly_min_t = dly_min_samples_t - (int)dly_min_samples_t;
    float dly_plus_t = dly_plus_samples_t - (int)dly_plus_samples_t;
    int dly_min_samples_0 = (int)dly_min_samples_t;
    int dly_min_samples_1 = (int)dly_min_samples_t + 1;
    int dly_plus_samples_0 = (int)dly_plus_samples_t;
    int dly_plus_samples_1 = (int)dly_plus_samples_t + 1;
    for (int c = 0; c < 2; c++)
    {
      // + 2*samples in case dly_samples > comb_samples
      float min0 = _comb_out[c][(_comb_pos + 2 * _comb_samples - dly_min_samples_0) % _comb_samples];
      float min1 = _comb_out[c][(_comb_pos + 2 * _comb_samples - dly_min_samples_1) % _comb_samples];
      float min = ((1 - dly_min_t) * min0 + dly_min_t * min1) * gain_min;
      float plus0 = _comb_in[c][(_comb_pos + 2 * _comb_samples - dly_plus_samples_0) % _comb_samples];
      float plus1 = _comb_in[c][(_comb_pos + 2 * _comb_samples - dly_plus_samples_1) % _comb_samples];
      float plus = ((1 - dly_plus_t) * plus0 + dly_plus_t * plus1) * gain_plus;
      _comb_in[c][_comb_pos] = block.state.own_audio[0][0][c][f];
      _comb_out[c][_comb_pos] = block.state.own_audio[0][0][c][f] + plus + min * feedback_factor;
      block.state.own_audio[0][0][c][f] = _comb_out[c][_comb_pos];
    }
    _comb_pos = (_comb_pos + 1) % _comb_samples;
  }
}

void
fx_engine::process_svf(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int svf_type = block_auto[param_svf_type][0].step();
  switch (svf_type)
  {
  case svf_type_lpf: process_svf_type(block, modulation, init_svf_lpf); break;
  case svf_type_hpf: process_svf_type(block, modulation, init_svf_hpf); break;
  case svf_type_bpf: process_svf_type(block, modulation, init_svf_bpf); break;
  case svf_type_bsf: process_svf_type(block, modulation, init_svf_bsf); break;
  case svf_type_apf: process_svf_type(block, modulation, init_svf_apf); break;
  case svf_type_peq: process_svf_type(block, modulation, init_svf_peq); break;
  case svf_type_bll: process_svf_type(block, modulation, init_svf_bll); break;
  case svf_type_lsh: process_svf_type(block, modulation, init_svf_lsh); break;
  case svf_type_hsh: process_svf_type(block, modulation, init_svf_hsh); break;
  default: assert(false); break;
  }
}

template <class Init> void
fx_engine::process_svf_type(plugin_block& block, cv_matrix_mixdown const& modulation, Init init)
{
  double a1, a2, a3;
  double m0, m1, m2;
  double w, hz, gain, kbd;

  double const max_res = 0.99;
  int this_module = _global ? module_gfx : module_vfx;
  auto const& res_curve = *modulation[this_module][block.module_slot][param_svf_res][0];
  auto const& kbd_curve = *modulation[this_module][block.module_slot][param_svf_kbd][0];
  auto const& freq_curve = *modulation[this_module][block.module_slot][param_svf_freq][0];
  auto const& gain_curve = *modulation[this_module][block.module_slot][param_svf_gain][0];

  int kbd_pivot = midi_middle_c;
  int kbd_current = _global ? (block.state.last_midi_note == -1 ? midi_middle_c : block.state.last_midi_note) : block.voice->state.id.key;
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    kbd = block.normalized_to_raw(this_module, param_svf_kbd, kbd_curve[f]);
    hz = block.normalized_to_raw(this_module, param_svf_freq, freq_curve[f]);
    gain = block.normalized_to_raw(this_module, param_svf_gain, gain_curve[f]);
    hz *= std::pow(2.0, (kbd_current - kbd_pivot) / 12.0 * kbd);
    hz = std::clamp(hz, flt_min_freq, flt_max_freq);

    w = pi64 * hz / block.sample_rate;
    init(w, res_curve[f] * max_res, gain, a1, a2, a3, m0, m1, m2);
    for (int c = 0; c < 2; c++)
    {
      double v0 = block.state.own_audio[0][0][c][f];
      double v3 = v0 - _ic2eq[c];
      double v1 = a1 * _ic1eq[c] + a2 * v3;
      double v2 = _ic2eq[c] + a2 * _ic1eq[c] + a3 * v3;
      _ic1eq[c] = 2 * v1 - _ic1eq[c];
      _ic2eq[c] = 2 * v2 - _ic2eq[c];
      block.state.own_audio[0][0][c][f] = m0 * v0 + m1 * v1 + m2 * v2;
    }
  }
}

template <bool Graph> void
fx_engine::process_shaper(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  switch (block_auto[param_shape_clip][0].step())
  {
  case shape_clip_clip: process_shaper_clip<Graph>(block, modulation, [](float in) { return std::clamp(in, -1.0f, 1.0f); }); break;
  case shape_clip_tanh: process_shaper_clip<Graph>(block, modulation, [](float in) { return std::tanh(in); }); break;
  default: assert(false); break;
  }
}

template <bool Graph, class Clip> void
fx_engine::process_shaper_clip(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip)
{
  switch (_shape_type_items[block.state.own_block_automation[param_shape_type][0].step()].index1)
  {
  case wave_shape_type_saw: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_saw); break;
  case wave_shape_type_sqr: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sqr); break;
  case wave_shape_type_tri: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_tri); break;
  case wave_shape_type_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin); break;
  case wave_shape_type_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos); break;
  case wave_shape_type_sin_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin_sin); break;
  case wave_shape_type_sin_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin_cos); break;
  case wave_shape_type_cos_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos_sin); break;
  case wave_shape_type_cos_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos_cos); break;
  case wave_shape_type_sin_sin_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin_sin_sin); break;
  case wave_shape_type_sin_sin_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin_sin_cos); break;
  case wave_shape_type_sin_cos_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin_cos_sin); break;
  case wave_shape_type_sin_cos_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_sin_cos_cos); break;
  case wave_shape_type_cos_sin_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos_sin_sin); break;
  case wave_shape_type_cos_sin_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos_sin_cos); break;
  case wave_shape_type_cos_cos_sin: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos_cos_sin); break;
  case wave_shape_type_cos_cos_cos: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_cos_cos_cos); break;
  case wave_shape_type_smooth_or_fold: process_shaper_clip_shape<Graph>(block, modulation, clip, wave_shape_bi_fold); break;
  default: assert(false); break;
  }
}

template <bool Graph, class Clip, class Shape> void
fx_engine::process_shaper_clip_shape(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip, Shape shape)
{
  switch (_shape_type_items[block.state.own_block_automation[param_shape_type][0].step()].index2)
  {
  case wave_skew_type_off: process_shaper_clip_shape_x<Graph>(block, modulation, clip, shape, wave_skew_bi_off); break;
  case wave_skew_type_lin: process_shaper_clip_shape_x<Graph>(block, modulation, clip, shape, wave_skew_bi_lin); break;
  case wave_skew_type_scu: process_shaper_clip_shape_x<Graph>(block, modulation, clip, shape, wave_skew_bi_scu); break;
  case wave_skew_type_scb: process_shaper_clip_shape_x<Graph>(block, modulation, clip, shape, wave_skew_bi_scb); break;
  case wave_skew_type_xpu: process_shaper_clip_shape_x<Graph>(block, modulation, clip, shape, wave_skew_bi_xpu); break;
  case wave_skew_type_xpb: process_shaper_clip_shape_x<Graph>(block, modulation, clip, shape, wave_skew_bi_xpb); break;
  default: assert(false); break;
  }
}

template <bool Graph, class Clip, class Shape, class SkewX> void
fx_engine::process_shaper_clip_shape_x(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x)
{
  switch (_shape_type_items[block.state.own_block_automation[param_shape_type][0].step()].index3)
  {
  case wave_skew_type_off: process_shaper_clip_shape_xy<Graph>(block, modulation, clip, shape, skew_x, wave_skew_bi_off); break;
  case wave_skew_type_lin: process_shaper_clip_shape_xy<Graph>(block, modulation, clip, shape, skew_x, wave_skew_bi_lin); break;
  case wave_skew_type_scu: process_shaper_clip_shape_xy<Graph>(block, modulation, clip, shape, skew_x, wave_skew_bi_scu); break;
  case wave_skew_type_scb: process_shaper_clip_shape_xy<Graph>(block, modulation, clip, shape, skew_x, wave_skew_bi_scb); break;
  case wave_skew_type_xpu: process_shaper_clip_shape_xy<Graph>(block, modulation, clip, shape, skew_x, wave_skew_bi_xpu); break;
  case wave_skew_type_xpb: process_shaper_clip_shape_xy<Graph>(block, modulation, clip, shape, skew_x, wave_skew_bi_xpb); break;
  default: assert(false); break;
  }
}

template <bool Graph, class Clip, class Shape, class SkewX, class SkewY> void 
fx_engine::process_shaper_clip_shape_xy(plugin_block& block, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x, SkewY skew_y)
{
  int this_module = _global ? module_gfx : module_vfx;
  auto const& block_auto = block.state.own_block_automation;
  int shape_type = block_auto[param_shape_type][0].step();
  int oversmp_stages = block_auto[param_shape_over][0].step();
  int oversmp_factor = 1 << oversmp_stages;

  auto const& type_item = _shape_type_items[shape_type];
  int sx = type_item.index2;
  int sy = type_item.index3;

  auto const& mix_curve = *modulation[this_module][block.module_slot][param_shape_mix][0];
  auto const& x_curve_plain = *modulation[this_module][block.module_slot][param_shape_x][0];
  auto const& y_curve_plain = *modulation[this_module][block.module_slot][param_shape_y][0];
  auto const& drive_curve_plain = *modulation[this_module][block.module_slot][param_shape_drive][0];

  jarray<float, 1> const* x_curve = &x_curve_plain;
  if(wave_skew_is_exp(sx))
  {
    auto& x_scratch = block.state.own_scratch[scratch_shape_x];
    for (int f = block.start_frame; f < block.end_frame; f++)
      x_scratch[f] = std::log(0.001 + (x_curve_plain[f] * 0.98)) / log_half;
    x_curve = &x_scratch;
  }

  jarray<float, 1> const* y_curve = &y_curve_plain;
  if (wave_skew_is_exp(sy))
  {
    auto& y_scratch = block.state.own_scratch[scratch_shape_y];
    for (int f = block.start_frame; f < block.end_frame; f++)
      y_scratch[f] = std::log(0.001 + (y_curve_plain[f] * 0.98)) / log_half;
    y_curve = &y_scratch;
  }

  auto& drive_curve = block.state.own_scratch[scratch_shape_drive_raw];
  normalized_to_raw_into(block, this_module, param_shape_drive, drive_curve_plain, drive_curve);

  // dont oversample for graphs
  if constexpr(Graph) oversmp_stages = 0;
  _shp_oversampler.process(oversmp_stages, block.state.own_audio[0][0], block.start_frame, block.end_frame, 
    [&block, &x_curve, &y_curve, &mix_curve, &drive_curve, this_module, clip, shape, skew_x, skew_y, oversmp_factor](int f, float in) {
      int mod_index = f / oversmp_factor;
      float shaped = clip(wave_calc_bi(in * drive_curve[mod_index], (*x_curve)[mod_index], (*y_curve)[mod_index], shape, skew_x, skew_y));
      return (1 - mix_curve[mod_index]) * in + mix_curve[mod_index] * shaped;
    });
  
  // dont dc filter for graphs
  if constexpr (!Graph) 
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
      {
        float mixed = block.state.own_audio[0][0][c][f];
        float filtered = mixed - _shp_dc_flt_x0[c] + _shp_dc_flt_y0[c] * _shp_dc_flt_r;
        _shp_dc_flt_x0[c] = mixed;
        _shp_dc_flt_y0[c] = filtered;
        block.state.own_audio[0][0][c][f] = filtered;
      }
}

}
