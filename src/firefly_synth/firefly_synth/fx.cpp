#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/trig.hpp>
#include <firefly_synth/synth.hpp>

#include <cmath>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth {

static double const comb_max_ms = 5;
static double const comb_min_ms = 0.1;
static double const svf_min_freq = 20;
static double const svf_max_freq = 20000;
static double const shp_pow_default_exp = 1;
static int const max_shp_cheby_chb = 16;

enum { shape_over_1, shape_over_2, shape_over_4, shape_over_8 };
enum { section_type, section_svf, section_comb, section_shape, section_delay };
enum { type_off,
  type_svf_lpf, type_svf_hpf, type_svf_bpf, type_svf_bsf, type_svf_apf, type_svf_peq, type_svf_bll, type_svf_lsh, type_svf_hsh, 
  type_shp_trig_sin, type_shp_trig_cos, type_shp_trig_sin_sin, type_shp_trig_sin_cos, type_shp_trig_sin_cos_lin, type_shp_trig_sin_cos_log, type_shp_trig_cos_sin, type_shp_trig_cos_cos,
  type_shp_trig_sin_sin_sin, type_shp_trig_sin_sin_cos, type_shp_trig_sin_cos_sin, type_shp_trig_sin_cos_cos,
  type_shp_trig_cos_sin_sin, type_shp_trig_cos_sin_cos, type_shp_trig_cos_cos_sin, type_shp_trig_cos_cos_cos,
  type_shp_other_clip, type_shp_other_tanh, type_shp_other_fold, type_shp_other_cbrt_clip, type_shp_other_cbrt_tanh, 
  type_shp_other_cube_clip, type_shp_other_cube_tanh, type_shp_other_pow_clip, type_shp_other_pow_tanh, type_shp_other_cheby_clip, type_shp_other_cheby_tanh,
  type_comb, type_delay };
enum { param_type, 
  param_svf_freq, param_svf_res, param_svf_kbd, param_svf_gain, 
  param_comb_dly_plus, param_comb_dly_min, param_comb_gain_plus, param_comb_gain_min,
  param_shape_over, param_shape_gain, param_shape_mix, param_shape_pow_exp, param_shape_cheby_chb,
  param_delay_tempo, param_delay_feedback };

static bool is_svf(int type) { return type_svf_lpf <= type && type <= type_svf_hsh; }
static bool is_svf_gain(int type) { return type_svf_bll <= type && type <= type_svf_hsh; }
static bool is_shape(int type) { return type_shp_trig_sin <= type && type <= type_shp_other_cheby_tanh; }
static bool is_shape_pow(int type) { return type_shp_other_pow_clip <= type && type <= type_shp_other_pow_tanh; }
static bool is_shape_cheby(int type) { return type_shp_other_cheby_clip <= type && type <= type_shp_other_cheby_tanh; }

static std::vector<list_item>
type_items(bool global)
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  
  result.emplace_back("{8B7E5C75-C3F6-4F53-900C-0A75703F5570}", "SVF.LPF");
  result.emplace_back("{673C872A-F740-431C-8CD3-F577CE984C2D}", "SVF.HPF");
  result.emplace_back("{9AADCB70-4753-400A-B4C3-A68AFA60273E}", "SVF.BPF");
  result.emplace_back("{3516F76F-1162-4180-AAE6-BA658A6035C6}", "SVF.BSF");
  result.emplace_back("{57BA905C-9735-4090-9D4D-75F6CD639387}", "SVF.APF");
  result.emplace_back("{13013D87-0DBA-47D7-BE59-F6B1B65CA118}", "SVF.PEQ");
  result.emplace_back("{463BAD99-6E33-4052-B6EF-31D6D781F002}", "SVF.BLL");
  result.emplace_back("{0ECA44F9-57AD-44F4-A066-60A166F4BD86}", "SVF.LSH");
  result.emplace_back("{D28FA8B1-3D45-4C80-BAA3-C6735FA4A5E2}", "SVF.HSH");
  
  result.emplace_back("{E8AB234B-871E-44CB-A903-D99F22532386}", "Shp.Sin");
  result.emplace_back("{84BB1282-F7EF-4F0C-82FA-9F1A27ADA849}", "Shp.Cos");
  result.emplace_back("{AA230E02-3391-4B83-A0D2-8AC7955E0554}", "Shp.SinSin");
  result.emplace_back("{E07D7F55-6861-442D-B9A2-7FFFBCCBF6B2}", "Shp.SinCos");
  result.emplace_back("{51A26A24-452D-42E7-A547-7FA74CEB9506}", "Shp.SinCosLin");
  result.emplace_back("{FBAF87E7-A287-4DA9-953C-003EFB49F5BD}", "Shp.SinCosLog");
  result.emplace_back("{78DE21F3-7786-4B5D-95D4-971C658E0598}", "Shp.CosSin");
  result.emplace_back("{9E4277B3-8208-4BE2-A728-80C8E1BF4FBF}", "Shp.CosCos");
  result.emplace_back("{8B231C79-EA39-4BBD-9F88-D5A4B62977BB}", "Shp.SinSinSin");
  result.emplace_back("{726BC81D-CE23-432B-9F72-D0A94D2712D3}", "Shp.SinSinCos");
  result.emplace_back("{45231645-D267-48B2-8E49-7AA93FDC26F7}", "Shp.SinCosSin");
  result.emplace_back("{7B75213E-261B-4E3A-B7A1-81D07F51C81B}", "Shp.SinCosCos");
  result.emplace_back("{CC0425A5-8C3B-4E97-9A48-18106FF46F94}", "Shp.CosSinSin");
  result.emplace_back("{70635CC4-E9C7-488C-8CA8-DA97A1394DAF}", "Shp.CosSinCos");
  result.emplace_back("{96553C46-B46D-46FA-9469-F32276F9CC52}", "Shp.CosCosSin");
  result.emplace_back("{3D2123DB-5089-4E4F-A737-83086AE30B0D}", "Shp.CosCosCos");

  result.emplace_back("{834AF6C3-DEBD-4B9D-8C84-B885B664EB04}", "Shp.Clip");
  result.emplace_back("{698E90E1-A422-4A22-970A-36659BD9B4BC}", "Shp.Tanh");
  result.emplace_back("{3ED95B95-C9D1-4D2E-A75B-19979D9E2841}", "Shp.Fold");
  result.emplace_back("{E33475AE-2548-42AF-B2CD-AA51E8E23543}", "Shp.CbrtClip");
  result.emplace_back("{B35997A3-9125-48A9-8166-FCE15B57A745}", "Shp.CbrtTanh");
  result.emplace_back("{0EE65A64-C81E-4E75-893C-9835BB88A705}", "Shp.CubeClip");
  result.emplace_back("{BCB5086C-3569-4122-84EC-9275FE5E39FC}", "Shp.CubeTanh");
  result.emplace_back("{32837427-F399-48C5-B6FD-8D4276E20822}", "Shp.PowClip");
  result.emplace_back("{C0781E48-31A2-4048-AF78-8EE3E8E98C1B}", "Shp.PowTanh");
  result.emplace_back("{1D5968AF-15AA-400C-B260-F4EACB9A56C5}", "Shp.ChbyClip");
  result.emplace_back("{DB0C51DF-6947-4C14-9222-102DB0A0CF9D}", "Shp.ChbyTanh");

  result.emplace_back("{8140F8BC-E4FD-48A1-B147-CD63E9616450}", "Comb");
  if(global) result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  return result;
}

static std::vector<list_item>
shape_over_items()
{
  std::vector<list_item> result;
  result.emplace_back("{AFE72C25-18F2-4DB5-A3F0-1A188032F6FB}", "OvrSmp.1X");
  result.emplace_back("{0E961515-1089-4E65-99C0-3A493253CF07}", "OvrSmp.2X");
  result.emplace_back("{59C0B496-3241-4D56-BE1F-D7B4B08DB64D}", "OvrSmp.4X");
  result.emplace_back("{BAA4877E-1A4A-4D71-8B80-1AC567B7A37B}", "OvrSmp.8X");
  return result;
}

class fx_engine: 
public module_engine {  

  bool const _global;

  // svf
  double _ic1eq[2];
  double _ic2eq[2];

  // shape
  float _shp_dc_avg = {};
  int _shp_prev_chb = -1;
  int _shp_prev_type = -1;

  // comb
  int _comb_pos = {};
  int _comb_samples = {};
  std::vector<double> _comb_in[2];
  std::vector<double> _comb_out[2];

  // delay
  int _dly_pos = {};
  int const _dly_capacity = {};
  jarray<float, 2> _dly_buffer = {};

  void update_shape_dc(int type, int chb);
  void process_comb(plugin_block& block, cv_matrix_mixdown const& modulation);
  void process_delay(plugin_block& block, cv_matrix_mixdown const& modulation);
  template <class Init> void process_svf(plugin_block& block, cv_matrix_mixdown const& modulation, Init init);
  template <class Shape> void process_shape(plugin_block& block, cv_matrix_mixdown const& modulation, Shape shape);

public:
  PB_PREVENT_ACCIDENTAL_COPY(fx_engine);
  fx_engine(bool global, int sample_rate);
  
  void reset(plugin_block const*) override;
  void process(plugin_block& block) override { process(block, nullptr, nullptr); }
  void process(plugin_block& block, cv_matrix_mixdown const* modulation, jarray<float, 2> const* audio_in);
};

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vfx, 0, param_type, 0, "SVF.LPF");
  state.set_text_at(module_vfx, 0, param_svf_res, 0, "50");
  state.set_text_at(module_vfx, 0, param_svf_freq, 0, "20");
}

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_gfx, 0, param_type, 0, "SVF.LPF");
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
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step();
  if(type == type_off) return graph_data(graph_data_type::off, {});

  int frame_count = -1;
  int sample_rate = -1;
  jarray<float, 2> audio_in;
  auto const params = make_graph_engine_params();
  if (is_shape(type))
  {
    frame_count = 200;
    sample_rate = 200;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for(int i = 0; i <= 199; i++)
      audio_in[0][i] = audio_in[1][i] = unipolar_to_bipolar((float)i / 199);
  } else if(is_svf(type) || type == type_comb)
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
    mapping.module_index, mapping.module_slot, [mapping, sample_rate, &audio_in](plugin_block& block) {
    bool global = mapping.module_index == module_gfx;
    fx_engine engine(global, sample_rate);
    engine.reset(&block);
    cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
    engine.process(block, &modulation, &audio_in);
  });
  engine->process_end();

  auto const& audio = block->state.own_audio[0][0][0];
  if (type == type_delay || is_shape(type))
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
  auto const voice_info = make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Voice FX", "V.FX", true, true, module_vfx, 10);
  auto const global_info = make_topo_info("{31EF3492-FE63-4A59-91DA-C2B4DD4A8891}", "Global FX", "G.FX", true, true, module_gfx, 10);
  module_stage stage = global ? module_stage::output : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{E7C21225-7ED5-45CC-9417-84A69BECA73C}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { gui_dimension::auto_size, 1 } })));
 
  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_graph_engine;
  if(global) result.default_initializer = init_global_default;
  if(!global) result.default_initializer = init_voice_default;
  result.gui.menu_handler_factory = [global](plugin_state* state) { return make_audio_routing_menu_handler(state, global); };
  result.engine_factory = [global](auto const&, int sample_rate, int) { return std::make_unique<fx_engine>(global, sample_rate); };

  result.sections.emplace_back(make_param_section(section_type,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0 }, { 1, 1 })));

  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "Type", param_type, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(type_items(global), ""),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  type.gui.submenu->indices.push_back(type_comb);
  type.gui.submenu->add_submenu("SVF", { type_svf_lpf, type_svf_hpf, type_svf_bpf, type_svf_bsf, type_svf_apf, type_svf_peq, type_svf_bll, type_svf_lsh, type_svf_hsh });
  type.gui.submenu->add_submenu("Shape.Trig", { 
    type_shp_trig_sin, type_shp_trig_cos, type_shp_trig_sin_sin, type_shp_trig_sin_cos, type_shp_trig_sin_cos_lin, type_shp_trig_sin_cos_log, type_shp_trig_cos_sin, type_shp_trig_cos_cos,
    type_shp_trig_sin_sin_sin, type_shp_trig_sin_sin_cos, type_shp_trig_sin_cos_sin, type_shp_trig_sin_cos_cos,
    type_shp_trig_cos_sin_sin, type_shp_trig_cos_sin_cos, type_shp_trig_cos_cos_sin, type_shp_trig_cos_cos_cos });
  type.gui.submenu->add_submenu("Shape.Other", { 
    type_shp_other_clip, type_shp_other_tanh, type_shp_other_fold, type_shp_other_cbrt_clip, type_shp_other_cbrt_tanh, type_shp_other_cube_clip, type_shp_other_cube_tanh,
    type_shp_other_pow_clip, type_shp_other_pow_tanh, type_shp_other_cheby_clip, type_shp_other_cheby_tanh });
  if(global) type.gui.submenu->indices.push_back(type_delay);

  auto& svf = result.sections.emplace_back(make_param_section(section_svf,
    make_topo_tag("{DFA6BD01-8F89-42CB-9D0E-E1902193DD5E}", "SVF"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 4, 1, 1, 1 } })));
  svf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf(vs[0]); });
  svf.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || is_svf(vs[0]); });
  auto& svf_freq = result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "SVF.Frq", "Frq", true, false, param_svf_freq, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(svf_min_freq, svf_max_freq, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 0 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf(vs[0]); });
  auto& svf_res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "SVF.Res", "Res", true, false, param_svf_res, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::knob, { 0, 1 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf(vs[0]); });
  auto& svf_kbd = result.params.emplace_back(make_param(
    make_topo_info("{9EEA6FE0-983E-4EC7-A47F-0DFD79D68BCB}", "SVF.Kbd", "Kbd", true, false, param_svf_kbd, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-2, 2, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_kbd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf(vs[0]); });
  auto& svf_gain = result.params.emplace_back(make_param(
    make_topo_info("{FE108A32-770A-415B-9C85-449ABF6A944C}", "SVF.Gain", "Gain", true, false, param_svf_gain, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(-24, 24, 0, 1, "dB"),
    make_param_gui_single(section_svf, gui_edit_type::knob, { 0, 3 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_gain.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf_gain(vs[0]); });

  auto& comb = result.sections.emplace_back(make_param_section(section_comb,
    make_topo_tag("{54CF060F-3EE7-4F42-921F-612F8EEA8EB0}", "Comb"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 2, 2, 1, 1 } })));
  comb.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_dly_plus = result.params.emplace_back(make_param(
    make_topo_info("{097ECBDB-1129-423C-9335-661D612A9945}", "Cmb.Dly+", "Dly+", true, false, param_comb_dly_plus, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_dly_plus.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_dly_min = result.params.emplace_back(make_param(
    make_topo_info("{D4846933-6AED-4979-AA1C-2DD80B68404F}", "Cmb.Dly-", "Dly-", true, false, param_comb_dly_min, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_dly_min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_gain_plus = result.params.emplace_back(make_param(
    make_topo_info("{3069FB5E-7B17-4FC4-B45F-A9DFA383CAA9}", "Cmb.Gain+", "Gain+", true, false, param_comb_gain_plus, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0.5, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::knob, { 0, 2 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_gain_plus.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  auto& comb_gain_min = result.params.emplace_back(make_param(
    make_topo_info("{9684165E-897B-4EB7-835D-D5AAF8E61E65}", "Cmb.Gain-", "Gain-", true, false, param_comb_gain_min, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_gain_min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });

  auto& shape = result.sections.emplace_back(make_param_section(section_shape,
    make_topo_tag("{4FD908CC-0EBA-4ADD-8622-EB95013CD429}", "Shape"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { gui_dimension::auto_size, 5, 5, 3 } })));
  shape.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return is_shape(vs[0]); });
  auto& shape_over = result.params.emplace_back(make_param(
    make_topo_info("{99C6E4A8-F90A-41DC-8AC7-4078A6DE0031}", "Shp.Over", "Over", true, false, param_shape_over, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(shape_over_items(), "OvrSmp.2X"),
    make_param_gui_single(section_shape, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  shape_over.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_shape(vs[0]); });
  auto& shape_gain = result.params.emplace_back(make_param(
    make_topo_info("{3FC57F28-075F-44A2-8D0D-6908447AE87C}", "Shp.Gain", "Gain", true, false, param_shape_gain, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(0.1, 32, 1, 1, 2, "%"),
    make_param_gui_single(section_shape, gui_edit_type::hslider, { 0, 1 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_gain.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_shape(vs[0]); });
  auto& shape_mix = result.params.emplace_back(make_param(
    make_topo_info("{667D9997-5BE1-48C7-9B50-4F178E2D9FE5}", "Shp.Mix", "Mix", true, false, param_shape_mix, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui_single(section_shape, gui_edit_type::hslider, { 0, 2 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_shape(vs[0]); });
  auto& shape_pow_exp = result.params.emplace_back(make_param(
    make_topo_info("{87DC7F98-8C73-4684-A2FE-8E4ABAD03A80}", "Shp.Exp", "Exp", true, false, param_shape_pow_exp, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(-32, 32, shp_pow_default_exp, 2, ""),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_pow_exp.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_shape_pow(vs[0]); });
  shape_pow_exp.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return !is_shape_cheby(vs[0]); });
  auto& shape_cheby_chb = result.params.emplace_back(make_param(
    make_topo_info("{0F9BF4C1-90BF-48A7-8893-489A5160A55A}", "Shp.Chb", "Chb", true, false, param_shape_cheby_chb, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_step(1, max_shp_cheby_chb, 4, 0),
    make_param_gui_single(section_shape, gui_edit_type::knob, { 0, 3 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  shape_cheby_chb.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_shape_cheby(vs[0]); });
  shape_cheby_chb.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return is_shape_cheby(vs[0]); });
  result.params[param_shape_pow_exp].gui.label_reference_text = result.params[param_shape_cheby_chb].info.tag.short_name;

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

static float shp_trig_sin(float in, float gain, float exp, int chb) { return sin_m11(in * gain); }
static float shp_trig_cos(float in, float gain, float exp, int chb) { return cos_m11(in * gain); }
static float shp_trig_sin_sin(float in, float gain, float exp, int chb) { return sin_sin_m11(in * gain); }
static float shp_trig_sin_cos(float in, float gain, float exp, int chb) { return sin_cos_m11(in * gain); }
static float shp_trig_cos_sin(float in, float gain, float exp, int chb) { return cos_sin_m11(in * gain); }
static float shp_trig_cos_cos(float in, float gain, float exp, int chb) { return cos_cos_m11(in * gain); }
static float shp_trig_sin_sin_sin(float in, float gain, float exp, int chb) { return sin_sin_sin_m11(in * gain); }
static float shp_trig_sin_sin_cos(float in, float gain, float exp, int chb) { return sin_sin_cos_m11(in * gain); }
static float shp_trig_sin_cos_sin(float in, float gain, float exp, int chb) { return sin_cos_sin_m11(in * gain); }
static float shp_trig_sin_cos_cos(float in, float gain, float exp, int chb) { return sin_cos_cos_m11(in * gain); }
static float shp_trig_cos_sin_sin(float in, float gain, float exp, int chb) { return sin_sin_sin_m11(in * gain); }
static float shp_trig_cos_sin_cos(float in, float gain, float exp, int chb) { return sin_sin_cos_m11(in * gain); }
static float shp_trig_cos_cos_sin(float in, float gain, float exp, int chb) { return sin_cos_sin_m11(in * gain); }
static float shp_trig_cos_cos_cos(float in, float gain, float exp, int chb) { return sin_cos_cos_m11(in * gain); }

static float shp_other_tanh(float in, float gain, float exp, int chb) { return std::tanh(in * gain); }
static float shp_other_clip(float in, float gain, float exp, int chb) { return std::clamp(in * gain, -1.0f, 1.0f); }
static float shp_other_cbrt_tanh(float in, float gain, float exp, int chb) { return std::tanh(std::cbrt(in * gain)); }
static float shp_other_cbrt_clip(float in, float gain, float exp, int chb) { return std::clamp(std::cbrt(in * gain), -1.0f, 1.0f); }
static float shp_other_cube_tanh(float in, float gain, float exp, int chb) { return std::tanh((in * gain) * (in * gain) * (in * gain)); }
static float shp_other_cube_clip(float in, float gain, float exp, int chb) { return std::clamp((in * gain) * (in * gain) * (in * gain), -1.0f, 1.0f); }
static float shp_other_pow_tanh(float in, float gain, float exp, int chb) { return std::tanh((in * gain < 0 ? -1 : 1) * std::pow(std::fabs(in * gain), exp)); }
static float shp_other_pow_clip(float in, float gain, float exp, int chb) { return std::clamp((in * gain < 0 ? -1 : 1) * std::pow(std::fabs(in * gain), exp), -1.0f, 1.0f); }

static float 
shp_other_fold(float in, float gain, float exp, int chb)
{
  float x = in * gain;
  while (true)
    if (x > 1.0f) x -= 2.0f * (x - 1.0f);
    else if (x < -1.0f) x += 2.0f * (-x - 1.0f);
    else return x;
  assert(false);
  return 0.0f;
}

static float
shp_other_cheby(float x, int chb)
{
  int terms = 1 + 2 * chb;
  std::array<float, 1 + 2 * max_shp_cheby_chb + 1> t;
  t[0] = 1.0f;
  t[1] = x;
  for (std::int32_t o = 2; o <= terms; o++)
    t[o] = 2.0f * t[1] * t[o - 1] - t[o - 2];
  return t[terms];
}

static float 
shp_other_cheby_tanh(float in, float gain, float exp, int chb)
{  return shp_other_cheby(std::tanh(in * gain), chb); }
static float
shp_other_cheby_clip(float in, float gain, float exp, int chb)
{  return shp_other_cheby(std::clamp(in * gain, -1.0f, 1.0f), chb); }

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
fx_engine(bool global, int sample_rate) :
_global(global), _dly_capacity(sample_rate * 10)
{ 
  _comb_samples = comb_max_ms * sample_rate * 0.001;
  _comb_in[0] = std::vector<double>(_comb_samples, 0.0);
  _comb_in[1] = std::vector<double>(_comb_samples, 0.0);
  _comb_out[0] = std::vector<double>(_comb_samples, 0.0);
  _comb_out[1] = std::vector<double>(_comb_samples, 0.0);
  if(global) _dly_buffer.resize(jarray<int, 1>(2, _dly_capacity));
}

void 
fx_engine::update_shape_dc(int type, int chb)
{
  // try to auto-adjust for dc offset introduced by non symmetric shapers
  // be sure not to introduce dc offset based on continuous parameters!
  // right now we calculate offset based on shaper type and chebyshev terms

  assert(is_shape(type));
  float (*shaper)(float in, float gain, float exp, int chb) = nullptr;
  switch (type)
  {
    case type_shp_other_tanh: shaper = shp_other_tanh; break;
    case type_shp_other_clip: shaper = shp_other_clip; break;
    case type_shp_other_fold: shaper = shp_other_fold; break;
    case type_shp_other_pow_tanh: shaper = shp_other_pow_tanh; break;
    case type_shp_other_pow_clip: shaper = shp_other_pow_clip; break;
    case type_shp_other_cube_tanh: shaper = shp_other_cube_tanh; break;
    case type_shp_other_cube_clip: shaper = shp_other_cube_clip; break;
    case type_shp_other_cbrt_tanh: shaper = shp_other_cbrt_tanh; break;
    case type_shp_other_cbrt_clip: shaper = shp_other_cbrt_clip; break;
    case type_shp_other_cheby_clip: shaper = shp_other_cheby_clip; break;
    case type_shp_other_cheby_tanh: shaper = shp_other_cheby_tanh; break;
    case type_shp_trig_sin: shaper = shp_trig_sin; break;
    case type_shp_trig_cos: shaper = shp_trig_cos; break;
    case type_shp_trig_sin_sin: shaper = shp_trig_sin_sin; break;
    case type_shp_trig_sin_cos: shaper = shp_trig_sin_cos; break;
    case type_shp_trig_sin_cos_lin: shaper = shp_trig_sin_cos; break;
    case type_shp_trig_sin_cos_log: shaper = shp_trig_sin_cos; break;
    case type_shp_trig_cos_sin: shaper = shp_trig_cos_sin; break;
    case type_shp_trig_cos_cos: shaper = shp_trig_cos_cos; break;
    case type_shp_trig_sin_sin_sin: shaper = shp_trig_sin_sin_sin; break;
    case type_shp_trig_sin_sin_cos: shaper = shp_trig_sin_sin_cos; break;
    case type_shp_trig_sin_cos_sin: shaper = shp_trig_sin_cos_sin; break;
    case type_shp_trig_sin_cos_cos: shaper = shp_trig_sin_cos_cos; break;
    case type_shp_trig_cos_sin_sin: shaper = shp_trig_cos_sin_sin; break;
    case type_shp_trig_cos_sin_cos: shaper = shp_trig_cos_sin_cos; break;
    case type_shp_trig_cos_cos_sin: shaper = shp_trig_cos_cos_sin; break;
    case type_shp_trig_cos_cos_cos: shaper = shp_trig_cos_cos_cos; break;
    default: assert(false); break;
  }

  // just hope a 100 samples does it
  _shp_dc_avg = 0;
  for (int f = 0; f < 100; f++)
  {
    float x = unipolar_to_bipolar((float)f / 100);
    float s = shaper(x, 1, shp_pow_default_exp, chb);
    _shp_dc_avg += s / 100;
  }
  check_bipolar(_shp_dc_avg);
}

void
fx_engine::reset(plugin_block const* block)
{
  _ic1eq[0] = 0;
  _ic1eq[1] = 0;
  _ic2eq[0] = 0;
  _ic2eq[1] = 0;

  _dly_pos = 0;
  _comb_pos = 0;

  auto const& block_auto = block->state.own_block_automation;
  int type = block_auto[param_type][0].step();
  int cheby_chb = block_auto[param_shape_cheby_chb][0].step();
  if (_global && type == type_delay) _dly_buffer.fill(0);
  if(!_global && is_shape(type)) update_shape_dc(type, cheby_chb);

  if (type == type_comb)
  {
    std::fill(_comb_in[0].begin(), _comb_in[0].end(), 0.0f);
    std::fill(_comb_in[1].begin(), _comb_in[1].end(), 0.0f);
    std::fill(_comb_out[0].begin(), _comb_out[0].end(), 0.0f);
    std::fill(_comb_out[1].begin(), _comb_out[1].end(), 0.0f);
  }    
}

void
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
  case type_comb: process_comb(block, *modulation); break;
  case type_delay: process_delay(block, *modulation); break;
  
  case type_svf_lpf: process_svf(block, *modulation, init_svf_lpf); break;
  case type_svf_hpf: process_svf(block, *modulation, init_svf_hpf); break;
  case type_svf_bpf: process_svf(block, *modulation, init_svf_bpf); break;
  case type_svf_bsf: process_svf(block, *modulation, init_svf_bsf); break;
  case type_svf_peq: process_svf(block, *modulation, init_svf_peq); break;
  case type_svf_apf: process_svf(block, *modulation, init_svf_apf); break;
  case type_svf_bll: process_svf(block, *modulation, init_svf_bll); break;
  case type_svf_lsh: process_svf(block, *modulation, init_svf_lsh); break;
  case type_svf_hsh: process_svf(block, *modulation, init_svf_hsh); break;

  case type_shp_other_tanh: process_shape(block, *modulation, shp_other_tanh); break;
  case type_shp_other_clip: process_shape(block, *modulation, shp_other_clip); break;
  case type_shp_other_fold: process_shape(block, *modulation, shp_other_fold); break;
  case type_shp_other_pow_tanh: process_shape(block, *modulation, shp_other_pow_tanh); break;
  case type_shp_other_pow_clip: process_shape(block, *modulation, shp_other_pow_clip); break;
  case type_shp_other_cube_tanh: process_shape(block, *modulation, shp_other_cube_tanh); break;
  case type_shp_other_cube_clip: process_shape(block, *modulation, shp_other_cube_clip); break;
  case type_shp_other_cbrt_tanh: process_shape(block, *modulation, shp_other_cbrt_tanh); break;
  case type_shp_other_cbrt_clip: process_shape(block, *modulation, shp_other_cbrt_clip); break;
  case type_shp_other_cheby_clip: process_shape(block, *modulation, shp_other_cheby_clip); break;
  case type_shp_other_cheby_tanh: process_shape(block, *modulation, shp_other_cheby_tanh); break;

  case type_shp_trig_sin: process_shape(block, *modulation, shp_trig_sin); break;
  case type_shp_trig_cos: process_shape(block, *modulation, shp_trig_cos); break;
  case type_shp_trig_sin_sin: process_shape(block, *modulation, shp_trig_sin_sin); break;
  case type_shp_trig_sin_cos: process_shape(block, *modulation, shp_trig_sin_cos); break;
  case type_shp_trig_sin_cos_lin: process_shape(block, *modulation, shp_trig_sin_cos); break;
  case type_shp_trig_sin_cos_log: process_shape(block, *modulation, shp_trig_sin_cos); break;
  case type_shp_trig_cos_sin: process_shape(block, *modulation, shp_trig_cos_sin); break;
  case type_shp_trig_cos_cos: process_shape(block, *modulation, shp_trig_cos_cos); break;
  case type_shp_trig_sin_sin_sin: process_shape(block, *modulation, shp_trig_sin_sin_sin); break;
  case type_shp_trig_sin_sin_cos: process_shape(block, *modulation, shp_trig_sin_sin_cos); break;
  case type_shp_trig_sin_cos_sin: process_shape(block, *modulation, shp_trig_sin_cos_sin); break;
  case type_shp_trig_sin_cos_cos: process_shape(block, *modulation, shp_trig_sin_cos_cos); break;
  case type_shp_trig_cos_sin_sin: process_shape(block, *modulation, shp_trig_cos_sin_sin); break;
  case type_shp_trig_cos_sin_cos: process_shape(block, *modulation, shp_trig_cos_sin_cos); break;
  case type_shp_trig_cos_cos_sin: process_shape(block, *modulation, shp_trig_cos_cos_sin); break;
  case type_shp_trig_cos_cos_cos: process_shape(block, *modulation, shp_trig_cos_cos_cos); break;

  default: assert(false); break;
  }
}

template <class Shape> void
fx_engine::process_shape(plugin_block& block, cv_matrix_mixdown const& modulation, Shape shape)
{
  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  int cheby_chb = block_auto[param_shape_cheby_chb][0].step();
  if (_global && (_shp_prev_type != type || _shp_prev_chb != cheby_chb))
  {
    _shp_prev_type = type;
    _shp_prev_chb = cheby_chb;
    update_shape_dc(type, cheby_chb);
  }

  int this_module = _global ? module_gfx : module_vfx;
  auto const& mix = *modulation[this_module][block.module_slot][param_shape_mix][0];
  auto const& exp_curve = *modulation[this_module][block.module_slot][param_shape_pow_exp][0];
  auto const& gain_curve = *modulation[this_module][block.module_slot][param_shape_gain][0];
  for(int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      float in = block.state.own_audio[0][0][c][f];
      float exp = block.normalized_to_raw(this_module, param_shape_gain, exp_curve[f]);
      float gain = block.normalized_to_raw(this_module, param_shape_gain, gain_curve[f]);
      float shaped = shape(in, gain, exp, cheby_chb);

      if(type == type_shp_trig_sin_cos_lin)
      {
        float shaped_offset = shaped - _shp_dc_avg;
        float shaped_dc_correct = std::clamp(shaped_offset, -1.0f, 1.0f);
        shaped_dc_correct = shaped;
        if(shaped > 0 && _shp_dc_avg > 0) shaped *= (1 - _shp_dc_avg);
        else if(shaped < 0 && _shp_dc_avg < 0) shaped *= (1 + _shp_dc_avg);
      }
      else if (type == type_shp_trig_sin_cos_log || type == type_shp_trig_cos_sin)
      {
        float shaped_uni = bipolar_to_unipolar(shaped);
        float offset_uni = bipolar_to_unipolar(_shp_dc_avg);
        //float shaped_dc_fix = std::pow(shaped_uni, std::log(1 - offset_uni) / std::log(0.5f));
        float shaped_dc_fix = std::pow(shaped_uni, std::log(offset_uni) / std::log(0.5f));
        shaped = unipolar_to_bipolar(shaped_dc_fix);
      }
      block.state.own_audio[0][0][c][f] = (1 - mix[f]) * in + mix[f] * shaped;
    }
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
    for(int c = 0; c < 2; c++)
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
  
template <class Init> void
fx_engine::process_svf(plugin_block& block, cv_matrix_mixdown const& modulation, Init init)
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
    hz = std::clamp(hz, svf_min_freq, svf_max_freq);

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

}
