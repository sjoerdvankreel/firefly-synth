#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/oversampler.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/svf.hpp>
#include <firefly_synth/synth.hpp>
#include <firefly_synth/waves.hpp>

#include <cmath>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth {

static float const log_half = std::log(0.5f);

static double const comb_max_ms = 5;
static double const comb_min_ms = 0.1;
static double const flt_min_freq = 20;
static double const flt_max_freq = 20000;
static double const dly_max_sec = 10;
static double const dly_max_filter_time_ms = 500;

static float const reverb_gain = 0.015f;
static float const reverb_dry_scale = 2.0f;
static float const reverb_wet_scale = 3.0f;
static float const reverb_damp_scale = 0.4f;
static float const reverb_room_scale = 0.28f;
static float const reverb_room_offset = 0.7f;
static float const reverb_spread = 23.0f / 44100.0f;

static int const reverb_comb_count = 8;
static int const reverb_allpass_count = 4;
static float const reverb_allpass_length[reverb_allpass_count] = {
  556.0f / 44100.0f, 441.0f / 44100.0f, 341.0f / 44100.0f, 225.0f / 44100.0f };
static float const reverb_comb_length[reverb_comb_count] = {
  1116.0f / 44100.0f, 1188.0f / 44100.0f, 1277.0f / 44100.0f, 1356.0f / 44100.0f,
  1422.0f / 44100.0f, 1491.0f / 44100.0f, 1557.0f / 44100.0f, 1617.0f / 44100.0f };

enum { dist_clip_clip, dist_clip_tanh };
enum { dist_over_1, dist_over_2, dist_over_4, dist_over_8, dist_over_16 };
enum { dly_type_fdbk_time, dly_type_fdbk_sync, dly_type_multi_time, dly_type_multi_sync };
enum { section_type, section_svf, section_comb, section_dist, section_delay, section_reverb };
enum { type_off, type_svf, type_cmb, type_dst_a, type_dst_b, type_dst_c, type_delay, type_reverb };
enum { svf_type_lpf, svf_type_hpf, svf_type_bpf, svf_type_bsf, svf_type_apf, svf_type_peq, svf_type_bll, svf_type_lsh, svf_type_hsh };

enum { scratch_dly_fdbk_l, scratch_dly_fdbk_r, scratch_dly_fdbk_count };
enum { scratch_dly_multi_hold, scratch_dly_multi_time, scratch_dly_multi_count };
enum { scratch_dist_x, scratch_dist_y, scratch_dist_gain_raw, scratch_dist_count };
enum { scratch_reverb_damp, scratch_reverb_size, scratch_reverb_in, scratch_reverb_count };
static int constexpr scratch_count = std::max({ (int)scratch_dly_fdbk_count, (int)scratch_dly_multi_count, (int)scratch_dist_count, (int)scratch_reverb_count });

enum { param_type,
  param_svf_type, param_svf_freq, param_svf_res, param_svf_gain, param_svf_kbd,
  param_comb_dly_plus, param_comb_gain_plus, param_comb_dly_min, param_comb_gain_min,
  param_dist_over, param_dist_clip, param_dist_shape, param_dist_x, param_dist_y, param_dist_lp_frq, param_dist_lp_res, param_dist_mix, param_dist_gain,
  param_dly_type, param_dly_amt, param_dly_sprd, param_dly_mix,
  param_dly_fdbk_time_l, param_dly_fdbk_tempo_l, param_dly_fdbk_time_r, param_dly_fdbk_tempo_r,
  param_dly_multi_time, param_dly_multi_tempo, param_dly_multi_taps,  
  param_dly_hold_time, param_dly_hold_tempo,
  param_reverb_size, param_reverb_damp, param_reverb_spread, param_reverb_apf, param_reverb_mix
};

static bool svf_has_gain(int svf_type) { return svf_type >= svf_type_bll; }
static bool dist_has_lpf(int type) { return type_dst_b <= type && type <= type_dst_c; }
static bool type_is_dist(int type) { return type_dst_a <= type && type <= type_dst_c; }
static bool dly_is_sync(int dly_type) { return dly_type == dly_type_fdbk_sync || dly_type == dly_type_multi_sync; }
static bool dly_is_multi(int dly_type) { return dly_type == dly_type_multi_time || dly_type == dly_type_multi_sync; }
static bool dst_has_skew_x(multi_menu const& menu, int type) { return menu.multi_items[type].index2 != wave_skew_type_off; }
static bool dst_has_skew_y(multi_menu const& menu, int type) { return menu.multi_items[type].index3 != wave_skew_type_off; }

static std::vector<list_item>
type_items(bool global)
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  result.emplace_back("{9CB55AC0-48CB-43ED-B81E-B97C08771815}", "SVF");
  result.emplace_back("{8140F8BC-E4FD-48A1-B147-CD63E9616450}", "Comb");
  result.emplace_back("{277BDD6B-C1F8-4C33-90DB-F4E144FE06A6}", "DstA");
  result.emplace_back("{6CCE41B3-3A74-4F6A-9AB1-660BF492C8E7}", "DstB");
  result.emplace_back("{4A7A2979-0E1F-49E9-87CC-6E82355CFEA7}", "DstC");
  if(!global) return result;
  result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  result.emplace_back("{7BB990E6-9A61-4C9F-BDAC-77D1CC260017}", "Reverb");
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
dly_type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{A1481F0B-D6FD-4375-BDF9-C01D2F5C5B79}", "Fdbk.Time");
  result.emplace_back("{7CEC3D1C-6854-4591-9AD7-BDBA9509EA87}", "Fdbk.Sync");
  result.emplace_back("{871622C7-EC8A-4E3B-A76C-CFDE3467A998}", "Multi.Time");
  result.emplace_back("{62EB5BA9-889A-4C46-8534-12881A4F02D1}", "Multi.Sync");
  return result;
}

static std::vector<list_item>
dist_clip_items()
{
  std::vector<list_item> result;
  result.emplace_back("{FAE2F1EB-248D-4BA2-A008-07C2CD56EB71}", "Clip");
  result.emplace_back("{E40AE5EA-2E84-436F-960E-2D8733F0AA42}", "Tanh");
  return result;
}

static std::vector<list_item>
dist_over_items()
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
  state_var_filter _svf;

  // comb
  int _comb_pos = {};
  int _comb_samples = {};
  std::vector<double> _comb_in[2];
  std::vector<double> _comb_out[2];

  // delay
  int _dly_pos = {};
  int const _dly_capacity = {};
  jarray<float, 2> _dly_buffer = {};

  // distortion with fixed dc filter @20hz
  // and resonant lp filter in the oversampling stage
  dc_filter _dst_dc;
  state_var_filter _dst_svf;
  oversampler<1> _dst_oversampler;
  std::vector<multi_menu_item> _dst_shape_items;

  // reverb
  // https://github.com/sinshu/freeverb
  std::array<std::array<float, reverb_comb_count>, 2> _rev_comb_filter = {};
  std::array<std::array<std::int32_t, reverb_comb_count>, 2> _rev_comb_pos = {};
  std::array<std::array<std::vector<float>, reverb_comb_count>, 2> _rev_comb = {};
  std::array<std::array<std::int32_t, reverb_allpass_count>, 2> _rev_allpass_pos = {};
  std::array<std::array<std::vector<float>, reverb_allpass_count>, 2> _rev_allpass = {};

  void process_comb(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
  void process_delay(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
  void process_reverb(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
  void process_dly_fdbk(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
  void process_dly_multi(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
 
  // https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
  void process_svf(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
  template <class Init> 
  void process_svf_type(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Init init);
  
  template <bool Graph, int Type>
  void process_dist(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation);
  template <bool Graph, int Type, class Clip>
  void process_dist_clip(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip);
  template <bool Graph, int Type, class Clip, class Shape>
  void process_dist_clip_shape(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip, Shape shape);
  template <bool Graph, int Type, class Clip, class Shape, class SkewX>
  void process_dist_clip_shape_x(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x);
  template <bool Graph, int Type, class Clip, class Shape, class SkewX, class SkewY>
  void process_dist_clip_shape_xy(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x, SkewY skew_y);
  void dist_svf_next(plugin_block const& block, int oversmp_factor, double freq_plain, double res, float& left, float& right);

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
  state.set_text_at(module_gfx, 1, param_dly_type, 0, "Fdbk.Sync");
  state.set_text_at(module_gfx, 1, param_dly_fdbk_tempo_l, 0, "3/16");
  state.set_text_at(module_gfx, 1, param_dly_fdbk_tempo_r, 0, "5/16");
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
  int const shp_cycle_count = 5;
  int const shp_cycle_length = 200;

  auto const params = make_graph_engine_params();
  if (type_is_dist(type))
  {
    // need many samples for filters to stabilize
    sample_rate = 48000;
    frame_count = shp_cycle_length * shp_cycle_count;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for(int i = 0; i < shp_cycle_count; i++)
      for (int j = 0; j < shp_cycle_length; j++)
        audio_in[0][i * shp_cycle_length + j] = audio_in[1][i * shp_cycle_length + j] 
          = unipolar_to_bipolar((float)j / (shp_cycle_length - 1));
  } else if(type == type_svf || type == type_cmb)
  {
    // need many samples for fft to work
    frame_count = 4800;
    sample_rate = 48000;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    audio_in[0][0] = 1;
    audio_in[1][0] = 1;
  } else if (type == type_delay)
  {
    // input is single sycle sine
    int count = 50;
    sample_rate = 200;
    frame_count = sample_rate * dly_max_sec;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for (int i = 0; i < count; i++)
    {
      float sample = std::sin(i / (float)count * pi32 * 2.0f);
      audio_in[0][i] = sample;
      audio_in[1][i] = sample;
    }
  }
  else
  {
    // need some more input than delay for reverb to have visual effect
    assert(type == type_reverb);
    int length = 450;
    sample_rate = 300;
    frame_count = 3000;
    float phase = 0.0f;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for (int i = 0; i < length; i++)
    {
      float sample = std::sin(phase * 2.0f * pi32) * (length - i) / length;
      phase += 60.0f / sample_rate;
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

  auto const& audio = block->state.own_audio[0][0];

  // comb / svf plot FR
  if (type == type_cmb || type == type_svf)
  {
    auto response = fft(audio[0].data());
    if (type == type_cmb)
      return graph_data(jarray<float, 1>(response), false, 1.0f, { "24 kHz" });

    // SVF remaps over 0.8 just to look pretty
    std::vector<float> response_mapped(log_remap_series_x(response, 0.8f));
    return graph_data(jarray<float, 1>(response_mapped), false, 1.0f, { "24 kHz" });
  }
  
  // distortion - pick result of the last cycle (after filters kick in)
  if (type_is_dist(type))
  {
    int last_cycle_start = (shp_cycle_count - 1) * shp_cycle_length;
    std::vector<float> series(audio[0].cbegin() + last_cycle_start, audio[0].cbegin() + frame_count);
    return graph_data(jarray<float, 1>(series), true, 1.0f, { "Distortion" });
  }

  // delay or reverb - do some autosizing so it looks pretty
  // note: we re-use dly_max_sec as max plotting time for the reverb
  assert(type == type_delay || type == type_reverb);
  float max_amp = 0.0f;
  int last_significant_frame = 0;
  for (int f = 0; f < frame_count; f++)
  {
    float amp_l = std::fabs(audio[0][f]);
    float amp_r = std::fabs(audio[1][f]);
    max_amp = std::max(max_amp, amp_l);
    max_amp = std::max(max_amp, amp_r);
    if(amp_l >= 1e-2) last_significant_frame = f;
    if(amp_r >= 1e-2) last_significant_frame = f;
  }

  if(max_amp == 0.0f) max_amp = 1.0f;
  if(last_significant_frame < frame_count / dly_max_sec) last_significant_frame = frame_count / dly_max_sec;
  std::vector<float> left(audio[0].cbegin(), audio[0].cbegin() + last_significant_frame + 1);
  std::vector<float> right(audio[1].cbegin(), audio[1].cbegin() + last_significant_frame + 1);
  for (int f = 0; f <= last_significant_frame; f++)
  {
    left[f] /= max_amp;
    right[f] /= max_amp;
  }

  float length = (last_significant_frame + 1.0f) / frame_count * dly_max_sec;
  std::string partition = float_to_string(length, 2) + " Sec";
  int dly_type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_dly_type, 0).step();
  if (type == type_delay && dly_is_sync(dly_type))
  {
    float one_bar_length = timesig_to_time(120, { 1, 1 });
    partition = float_to_string(length / one_bar_length, 2) + " Bar";
  }
  float stroke = type == type_delay? 1.0f: 0.25f;
  return graph_data(jarray<float, 2>(
    std::vector<jarray<float, 1>>({ jarray<float, 1>(left), jarray<float, 1>(right) })), 
    stroke, { partition });
}

module_topo
fx_topo(int section, gui_colors const& colors, gui_position const& pos, bool global)
{
  auto dist_shape_menu = make_wave_multi_menu(true);
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
  result.engine_factory = [global, shape_type_items = dist_shape_menu.multi_items](auto const&, int sample_rate, int max_frame_count) {
    return std::make_unique<fx_engine>(global, sample_rate, max_frame_count, shape_type_items); };
  result.graph_renderer = [shape_type_items = dist_shape_menu.multi_items](auto const& state, auto* engine, int param, auto const& mapping) {
      return render_graph(state, engine, param, mapping, shape_type_items); };

  result.sections.emplace_back(make_param_section(section_type,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0 }, { 1, 1 })));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "Type", param_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(type_items(global), ""),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  type.gui.submenu->add_submenu("Filter", { type_svf, type_cmb });
  type.gui.submenu->add_submenu("Distortion", { type_dst_a, type_dst_b, type_dst_c });
  if (global) type.gui.submenu->indices.push_back(type_delay);
  if (global) type.gui.submenu->indices.push_back(type_reverb);

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
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(flt_min_freq, flt_max_freq, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 1 }, 
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  auto& svf_res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "SVF.Res", "Res", true, false, param_svf_res, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  auto& svf_gain = result.params.emplace_back(make_param(
    make_topo_info("{FE108A32-770A-415B-9C85-449ABF6A944C}", "SVF.Gain", "Gain", true, false, param_svf_gain, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-24, 24, 0, 1, "dB"),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_gain.gui.bindings.enabled.bind_params({ param_type, param_svf_type }, [](auto const& vs) { return vs[0] == type_svf && svf_has_gain(vs[1]); });
  auto& svf_kbd = result.params.emplace_back(make_param(
    make_topo_info("{9EEA6FE0-983E-4EC7-A47F-0DFD79D68BCB}", "SVF.Kbd", "Kbd", true, false, param_svf_kbd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-2, 2, 1, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  svf_kbd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });

  auto& comb = result.sections.emplace_back(make_param_section(section_comb,
    make_topo_tag("{54CF060F-3EE7-4F42-921F-612F8EEA8EB0}", "Comb"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 1, 1, 1, 1 } })));
  comb.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  auto& comb_dly_plus = result.params.emplace_back(make_param(
    make_topo_info("{097ECBDB-1129-423C-9335-661D612A9945}", "Cmb.Dly+", "Dly+", true, false, param_comb_dly_plus, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_dly_plus.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  auto& comb_gain_plus = result.params.emplace_back(make_param(
    make_topo_info("{3069FB5E-7B17-4FC4-B45F-A9DFA383CAA9}", "Cmb.Gain+", "Gain+", true, false, param_comb_gain_plus, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0.5, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_gain_plus.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  auto& comb_dly_min = result.params.emplace_back(make_param(
    make_topo_info("{D4846933-6AED-4979-AA1C-2DD80B68404F}", "Cmb.Dly-", "Dly-", true, false, param_comb_dly_min, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_dly_min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  auto& comb_gain_min = result.params.emplace_back(make_param(
    make_topo_info("{9684165E-897B-4EB7-835D-D5AAF8E61E65}", "Cmb.Gain-", "Gain-", true, false, param_comb_gain_min, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  comb_gain_min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });

  auto& distortion = result.sections.emplace_back(make_param_section(section_dist,
    make_topo_tag("{4FD908CC-0EBA-4ADD-8622-EB95013CD429}", "Dist"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 
      gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size,
      gui_dimension::auto_size, gui_dimension::auto_size, 1 } })));
  distortion.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]); });
  auto& dist_over = result.params.emplace_back(make_param(
    make_topo_info("{99C6E4A8-F90A-41DC-8AC7-4078A6DE0031}", "Dst.OverSmp", "Dst.OverSmp", true, false, param_dist_over, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_over_items(), ""),
    make_param_gui_single(section_dist, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  dist_over.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]); });
  auto& dist_clip = result.params.emplace_back(make_param(
    make_topo_info("{810325E4-C3AB-48DA-A770-65887DF57845}", "Dst.Clip", "Clip", true, false, param_dist_clip, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_clip_items(), "Clip"),
    make_param_gui_single(section_dist, gui_edit_type::autofit_list, { 0, 1 }, make_label_none())));
  dist_clip.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]); });
  auto& dist_shape = result.params.emplace_back(make_param(
    make_topo_info("{BFB5A04F-5372-4259-8198-6761BA52ADEB}", "Dst.Shape.SkewX/SkewY", param_dist_shape, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_shape_menu.items, "Sin.Off/Off"),
    make_param_gui_single(section_dist, gui_edit_type::autofit_list, { 0, 2 }, make_label_none())));
  dist_shape.gui.submenu = dist_shape_menu.submenu;
  dist_shape.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]); });
  auto& dist_x = result.params.emplace_back(make_param(
    make_topo_info("{94A94B06-6217-4EF5-8BA1-9F77AE54076B}", "Dst.X", "X", true, false, param_dist_x, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dist, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dist_x.gui.bindings.enabled.bind_params({ param_type, param_dist_shape }, [dist_shape_menu](auto const& vs) {
    return type_is_dist(vs[0]) && dst_has_skew_x(dist_shape_menu, vs[1]); });
  auto& dist_y = result.params.emplace_back(make_param(
    make_topo_info("{042570BF-6F02-4F91-9805-6C49FE9A3954}", "Dst.Y", "Y", true, false, param_dist_y, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dist, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dist_y.gui.bindings.enabled.bind_params({ param_type, param_dist_shape }, [dist_shape_menu](auto const& vs) {
    return type_is_dist(vs[0]) && dst_has_skew_y(dist_shape_menu, vs[1]); });
  auto& dist_lp = result.params.emplace_back(make_param(
    make_topo_info("{C82BC20D-2F1E-4001-BCFB-0C8945D1B329}", "Dst.LPF", "LPF", true, false, param_dist_lp_frq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(flt_min_freq, flt_max_freq, flt_max_freq, 1000, 0, "Hz"),
    make_param_gui_single(section_dist, gui_edit_type::knob, { 0, 5 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dist_lp.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]) && dist_has_lpf(vs[0]); });
  auto& dist_res = result.params.emplace_back(make_param(
    make_topo_info("{A9F6D41F-3C99-44DD-AAAA-BDC1FEEFB250}", "Dst.Res", "Res", true, false, param_dist_lp_res, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_dist, gui_edit_type::knob, { 0, 6 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dist_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]) && dist_has_lpf(vs[0]); });
  auto& dist_mix = result.params.emplace_back(make_param(
    make_topo_info("{667D9997-5BE1-48C7-9B50-4F178E2D9FE5}", "Dst.Mix", "Mix", true, false, param_dist_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui_single(section_dist, gui_edit_type::knob, { 0, 7 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dist_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]); });
  auto& dist_gain = result.params.emplace_back(make_param(
    make_topo_info("{3FC57F28-075F-44A2-8D0D-6908447AE87C}", "Dst.Gain", "Gain", true, false, param_dist_gain, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(0.1, 32, 1, 1, 2, "%"),
    make_param_gui_single(section_dist, gui_edit_type::hslider, { 0, 8 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  dist_gain.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dist(vs[0]); });

  // delay lines and reverb global only, per-voice uses too much memory
  if(!global) return result;

  auto& delay = result.sections.emplace_back(make_param_section(section_delay,
    make_topo_tag("{E92225CF-21BF-459C-8C9D-8E50285F26D4}", "Delay"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 1, 1, 1 } })));
  delay.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_type = result.params.emplace_back(make_param(
    make_topo_info("{C2E282BA-9E4F-4AE6-A055-8B5456780C66}", "Dly.Type", "Type", true, false, param_dly_type, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_item(dly_type_items(), ""),
    make_param_gui_single(section_delay, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  delay_type.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_type.gui.submenu = std::make_shared<gui_submenu>();
  delay_type.gui.submenu->add_submenu("Feedback", { dly_type_fdbk_time, dly_type_fdbk_sync });
  delay_type.gui.submenu->add_submenu("Multi Tap", { dly_type_multi_time, dly_type_multi_sync });
  auto& delay_amt = result.params.emplace_back(make_param(
    make_topo_info("{7CEE3B9A-99CF-46D3-847B-42F91A4F5227}", "Dly.Amt", "Amt", true, false, param_dly_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_delay, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_amt.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_sprd = result.params.emplace_back(make_param(
    make_topo_info("{1BD8008B-DC2C-4A77-A5DE-869983E5786C}", "Dly.Spr", "Spr", true, false, param_dly_sprd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_delay, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_sprd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_mix = result.params.emplace_back(make_param(
    make_topo_info("{6933B1F7-886F-41F0-8D23-175AA537327E}", "Dly.Mix", "Mix", true, false, param_dly_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_delay, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });

  auto& delay_fdbk_time_l = result.params.emplace_back(make_param(
    make_topo_info("{E32F17BC-03D2-4F2D-8292-2B4C3AB24E8D}", "Dly.TimeL", "L", true, false, param_dly_fdbk_time_l, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 1, 1, 3, "Sec"),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_fdbk_time_l.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return !dly_is_sync(vs[0]) && !dly_is_multi(vs[0]); });
  delay_fdbk_time_l.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && !dly_is_sync(vs[1]) && !dly_is_multi(vs[1]); });
  auto& delay_fdbk_tempo_l = result.params.emplace_back(make_param(
    make_topo_info("{33BCF50C-C7DE-4630-A835-44D50DA3B8BB}", "Dly.TempoL", "L", true, false, param_dly_fdbk_tempo_l, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(false, { 4, 1 }, { 3, 16 }),
    make_param_gui_single(section_delay, gui_edit_type::list, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_fdbk_tempo_l.gui.submenu = make_timesig_submenu(delay_fdbk_tempo_l.domain.timesigs);
  delay_fdbk_tempo_l.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return dly_is_sync(vs[0]) && !dly_is_multi(vs[0]); });
  delay_fdbk_tempo_l.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && dly_is_sync(vs[1]) && !dly_is_multi(vs[1]); });
  auto& delay_fdbk_time_r = result.params.emplace_back(make_param(
    make_topo_info("{5561243C-838F-4C33-BD46-3E934E854969}", "Dly.TimeR", "R", true, false, param_dly_fdbk_time_r, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 1, 1, 3, "Sec"),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 5 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_fdbk_time_r.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return !dly_is_sync(vs[0]) && !dly_is_multi(vs[0]); });
  delay_fdbk_time_r.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && !dly_is_sync(vs[1]) && !dly_is_multi(vs[1]); });
  auto& delay_fdbk_tempo_r = result.params.emplace_back(make_param(
    make_topo_info("{4FA78F9E-AC3A-45D7-A8A3-E0E2C7C264D7}", "Dly.TempoR", "R", true, false, param_dly_fdbk_tempo_r, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(false, { 4, 1 }, { 3, 16 }),
    make_param_gui_single(section_delay, gui_edit_type::list, { 0, 5 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_fdbk_tempo_r.gui.submenu = make_timesig_submenu(delay_fdbk_tempo_r.domain.timesigs);
  delay_fdbk_tempo_r.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return dly_is_sync(vs[0]) && !dly_is_multi(vs[0]); });
  delay_fdbk_tempo_r.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && dly_is_sync(vs[1]) && !dly_is_multi(vs[1]); });

  auto& delay_multi_time = result.params.emplace_back(make_param(
    make_topo_info("{8D1A0D44-3291-488F-AC86-9B2B608F9562}", "Dly.Time", "Time", true, false, param_dly_multi_time, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 1, 1, 3, "Sec"),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_multi_time.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return !dly_is_sync(vs[0]) && dly_is_multi(vs[0]); });
  delay_multi_time.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && !dly_is_sync(vs[1]) && dly_is_multi(vs[1]); });
  auto& delay_multi_tempo = result.params.emplace_back(make_param(
    make_topo_info("{8DAED046-7F5F-4E76-A6BF-099510564500}", "Dly.Tempo", "Tempo", true, false, param_dly_multi_tempo, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(false, { 4, 1 }, { 3, 16 }),
    make_param_gui_single(section_delay, gui_edit_type::list, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_multi_tempo.gui.submenu = make_timesig_submenu(delay_multi_tempo.domain.timesigs);
  delay_multi_tempo.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return dly_is_sync(vs[0]) && dly_is_multi(vs[0]); });
  delay_multi_tempo.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && dly_is_sync(vs[1]) && dly_is_multi(vs[1]); });
  auto& delay_multi_steps = result.params.emplace_back(make_param(
    make_topo_info("{27572912-0A8E-4A97-9A54-379829E8E794}", "Dly.Taps", "Taps", true, false, param_dly_multi_taps, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_step(1, 8, 4, 0),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 5 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_multi_steps.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return dly_is_multi(vs[0]); });
  delay_multi_steps.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && dly_is_multi(vs[1]); });

  auto& delay_hold_time = result.params.emplace_back(make_param(
    make_topo_info("{037E4A64-8F80-4E0A-88A0-EE1BB83C99C6}", "Dly.HoldTime", "Hld", true, false, param_dly_hold_time, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 0, 1, 3, "Sec"),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 6 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_hold_time.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return !dly_is_sync(vs[0]); });
  delay_hold_time.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && dly_is_multi(vs[1]); });
  auto& delay_hold_tempo = result.params.emplace_back(make_param(
    make_topo_info("{AED0D3A5-AB02-441F-A42D-7E2AEE88DF24}", "Dly.HoldTempo", "Hld", true, false, param_dly_hold_tempo, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(true, { 4, 1 }, { 0, 1 }),
    make_param_gui_single(section_delay, gui_edit_type::list, { 0, 6 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  delay_hold_tempo.gui.submenu = make_timesig_submenu(delay_hold_tempo.domain.timesigs);
  delay_hold_tempo.gui.bindings.visible.bind_params({ param_dly_type }, [](auto const& vs) { return dly_is_sync(vs[0]); });
  delay_hold_tempo.gui.bindings.enabled.bind_params({ param_type, param_dly_type }, [](auto const& vs) { return vs[0] == type_delay && dly_is_multi(vs[1]); });

  auto& reverb = result.sections.emplace_back(make_param_section(section_reverb,
    make_topo_tag("{92EFDFE7-41C5-4E9D-9BE6-DC56965C1C0D}", "Reverb"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 1, 1, 1, 1, 1 } })));
  reverb.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_size = result.params.emplace_back(make_param(
    make_topo_info("{E413FA18-420D-4510-80D1-54E2A0ED4CB2}", "Rev.Size", "Size", true, false, param_reverb_size, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.8, 0, true),
    make_param_gui_single(section_reverb, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  reverb_size.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_damp = result.params.emplace_back(make_param(
    make_topo_info("{44EE5538-9920-4F39-A68E-51E86E96943B}", "Rev.Damp", "Damp", true, false, param_reverb_damp, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.8, 0, true),
    make_param_gui_single(section_reverb, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  reverb_damp.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_sprd = result.params.emplace_back(make_param(
    make_topo_info("{0D138920-65D2-42E9-98C5-D8FEC5FD2C55}", "Rev.Sprd", "Sprd", true, false, param_reverb_spread, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_reverb, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  reverb_sprd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_apf = result.params.emplace_back(make_param(
    make_topo_info("{09DF58B0-4155-47F2-9AEB-927B2D8FD250}", "Rev.APF", "APF", true, false, param_reverb_apf, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1.0, 0, true),
    make_param_gui_single(section_reverb, gui_edit_type::hslider, { 0, 3 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  reverb_apf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_mix = result.params.emplace_back(make_param(
    make_topo_info("{7F71B450-2EAA-4D4E-8919-A94D87645DB0}", "Rev.Mix", "Mix", true, false, param_reverb_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_reverb, gui_edit_type::hslider, { 0, 4 },
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  reverb_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });

  return result;
}

fx_engine::
fx_engine(bool global, int sample_rate, int max_frame_count, std::vector<multi_menu_item> const& dst_shape_items) :
_global(global), _dly_capacity(sample_rate * dly_max_sec), 
_dst_oversampler(max_frame_count, false, false, false),
_dst_shape_items(dst_shape_items)
{ 
  _comb_samples = comb_max_ms * sample_rate * 0.001;
  _comb_in[0] = std::vector<double>(_comb_samples, 0.0);
  _comb_in[1] = std::vector<double>(_comb_samples, 0.0);
  _comb_out[0] = std::vector<double>(_comb_samples, 0.0);
  _comb_out[1] = std::vector<double>(_comb_samples, 0.0);
  
  if(!global) return;
  _dly_buffer.resize(jarray<int, 1>(2, _dly_capacity));
  for (int i = 0; i < reverb_comb_count; i++)
  {
    float comb_length_l = reverb_comb_length[i] * sample_rate;
    float comb_length_r = (reverb_comb_length[i] + reverb_spread) * sample_rate;
    _rev_comb[0][i] = std::vector<float>((int)comb_length_l);
    _rev_comb[1][i] = std::vector<float>((int)comb_length_r);
  }
  for (int i = 0; i < reverb_allpass_count; i++)
  {
    float apf_length_l = reverb_allpass_length[i] * sample_rate;
    float apf_length_r = (reverb_allpass_length[i] + reverb_spread) * sample_rate;
    _rev_allpass[0][i] = std::vector<float>((int)apf_length_l);
    _rev_allpass[1][i] = std::vector<float>((int)apf_length_r);
  }
}

void
fx_engine::reset(plugin_block const* block)
{
  _svf.clear();
  _dly_pos = 0;
  _comb_pos = 0;

  _dst_svf.clear();
  _dst_dc.init(block->sample_rate, 20);
  
  auto const& block_auto = block->state.own_block_automation;
  int type = block_auto[param_type][0].step();
  if (type == type_cmb)
  {
    std::fill(_comb_in[0].begin(), _comb_in[0].end(), 0.0f);
    std::fill(_comb_in[1].begin(), _comb_in[1].end(), 0.0f);
    std::fill(_comb_out[0].begin(), _comb_out[0].end(), 0.0f);
    std::fill(_comb_out[1].begin(), _comb_out[1].end(), 0.0f);
  }    

  if(!_global) return;
  if (type == type_delay) 
  {
    _dly_buffer[0].fill(0);
    _dly_buffer[1].fill(0);
  }
  if (type == type_reverb)
    for(int c = 0; c < 2; c++)
    {
      for (int i = 0; i < reverb_comb_count; i++)
      {
        _rev_comb_pos[c][i] = 0;
        _rev_comb_filter[c][i] = 0.0f;
        std::fill(_rev_comb[c][i].begin(), _rev_comb[c][i].end(), 0.0f);
      }
      for (int i = 0; i < reverb_allpass_count; i++)
      {
        _rev_allpass_pos[c][i] = 0;
        std::fill(_rev_allpass[c][i].begin(), _rev_allpass[c][i].end(), 0.0f);
      }
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
   
  int type = block.state.own_block_automation[param_type][0].step();  
  if(type == type_off)
  {
    for (int c = 0; c < 2; c++)
      (*audio_in)[c].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][c]);
    return;
  }

  if (modulation == nullptr) 
    modulation = &get_cv_matrix_mixdown(block, _global);  

  switch (type)
  {
  case type_svf: process_svf(block, *audio_in, *modulation); break;
  case type_cmb: process_comb(block, *audio_in, *modulation); break;
  case type_delay: process_delay(block, *audio_in, *modulation); break;
  case type_reverb: process_reverb(block, *audio_in, *modulation); break;
  case type_dst_a: process_dist<Graph, type_dst_a>(block, *audio_in, *modulation); break;
  case type_dst_b: process_dist<Graph, type_dst_b>(block, *audio_in, *modulation); break;
  case type_dst_c: process_dist<Graph, type_dst_c>(block, *audio_in, *modulation); break;
  default: assert(false); break;
  }
}

void
fx_engine::process_comb(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  float const feedback_factor = 0.98;
  int this_module = _global ? module_gfx : module_vfx;
  auto const& dly_min_curve = *modulation[this_module][block.module_slot][param_comb_dly_min][0];
  auto const& dly_plus_curve = *modulation[this_module][block.module_slot][param_comb_dly_plus][0];
  auto const& gain_min_curve = *modulation[this_module][block.module_slot][param_comb_gain_min][0];
  auto const& gain_plus_curve = *modulation[this_module][block.module_slot][param_comb_gain_plus][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float dly_min = block.normalized_to_raw_fast<domain_type::linear>(this_module, param_comb_dly_min, dly_min_curve[f]);
    float dly_plus = block.normalized_to_raw_fast<domain_type::linear>(this_module, param_comb_dly_plus, dly_plus_curve[f]);
    float gain_min = block.normalized_to_raw_fast<domain_type::linear>(this_module, param_comb_gain_min, gain_min_curve[f]);
    float gain_plus = block.normalized_to_raw_fast<domain_type::linear>(this_module, param_comb_gain_plus, gain_plus_curve[f]);
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
      _comb_in[c][_comb_pos] = audio_in[c][f];
      _comb_out[c][_comb_pos] = audio_in[c][f] + plus + min * feedback_factor;
      block.state.own_audio[0][0][c][f] = _comb_out[c][_comb_pos];
    }
    _comb_pos = (_comb_pos + 1) % _comb_samples;
  }
}

void
fx_engine::process_reverb(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  auto& scratch_in = block.state.own_scratch[scratch_reverb_in];
  auto& scratch_size = block.state.own_scratch[scratch_reverb_size];
  auto& scratch_damp = block.state.own_scratch[scratch_reverb_damp];

  auto const& apf_curve = *modulation[module_gfx][block.module_slot][param_reverb_apf][0];
  auto const& mix_curve = *modulation[module_gfx][block.module_slot][param_reverb_mix][0];
  auto const& size_curve = *modulation[module_gfx][block.module_slot][param_reverb_size][0];
  auto const& damp_curve = *modulation[module_gfx][block.module_slot][param_reverb_damp][0];
  auto const& spread_curve = *modulation[module_gfx][block.module_slot][param_reverb_spread][0];

  // Precompute channel independent stuff.
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    // Size doesnt respond linear.
    // By just testing by listening 80% is about the midpoint.
    // Do cube root so that 0.5 ~= 0.79.
    scratch_damp[f] = (1.0f - damp_curve[f]) * reverb_damp_scale;
    scratch_in[f] = (audio_in[0][f] + audio_in[1][f]) * reverb_gain;
    scratch_size[f] = (std::cbrt(size_curve[f]) * reverb_room_scale) + reverb_room_offset;
  }

  // Apply reverb per channel.
  for (int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][c][f] = 0.0f;
      for (int i = 0; i < reverb_comb_count; i++)
      {
        int pos = _rev_comb_pos[c][i];
        float comb = _rev_comb[c][i][pos];
        int length = _rev_comb[c][i].size();
        _rev_comb_filter[c][i] = (comb * (1.0f - scratch_damp[f])) + (_rev_comb_filter[c][i] * scratch_damp[f]);
        _rev_comb[c][i][pos] = scratch_in[f] + (_rev_comb_filter[c][i] * scratch_size[f]);
        _rev_comb_pos[c][i] = (pos + 1) % length;
        block.state.own_audio[0][0][c][f] += comb;
      }

      for (int i = 0; i < reverb_allpass_count; i++)
      {
        float output = block.state.own_audio[0][0][c][f];
        int pos = _rev_allpass_pos[c][i];
        float allpass = _rev_allpass[c][i][pos];
        int length = _rev_allpass[c][i].size();
        block.state.own_audio[0][0][c][f] = -block.state.own_audio[0][0][c][f] + allpass;
        _rev_allpass[c][i][pos] = output + (allpass * apf_curve[f] * 0.5f);
        _rev_allpass_pos[c][i] = (pos + 1) % length;
      }
    }

  // Final output depends on L/R both.
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float wet = mix_curve[f] * reverb_wet_scale;
    float dry = (1.0f - mix_curve[f]) * reverb_dry_scale;
    float wet1 = wet * (spread_curve[f] / 2.0f + 0.5f);
    float wet2 = wet * ((1.0f - spread_curve[f]) / 2.0f);
    float out_l = block.state.own_audio[0][0][0][f] * wet1 + block.state.own_audio[0][0][1][f] * wet2;
    float out_r = block.state.own_audio[0][0][1][f] * wet1 + block.state.own_audio[0][0][0][f] * wet2;
    block.state.own_audio[0][0][0][f] = out_l + audio_in[0][f] * dry;
    block.state.own_audio[0][0][1][f] = out_r + audio_in[1][f] * dry;
  }
}

void
fx_engine::process_svf(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int svf_type = block_auto[param_svf_type][0].step();
  switch (svf_type)
  {
  case svf_type_lpf: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_lpf(w, res); }); break;
  case svf_type_hpf: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_hpf(w, res); }); break;
  case svf_type_bpf: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_bpf(w, res); }); break;
  case svf_type_bsf: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_bsf(w, res); }); break;
  case svf_type_apf: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_apf(w, res); }); break;
  case svf_type_peq: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_peq(w, res); }); break;
  case svf_type_bll: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_bll(w, res, gn); }); break;
  case svf_type_lsh: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_lsh(w, res, gn); }); break;
  case svf_type_hsh: process_svf_type(block, audio_in, modulation, [this](double w, double res, double gn) { _svf.init_hsh(w, res, gn); }); break;
  default: assert(false); break;
  }
}

template <class Init> void
fx_engine::process_svf_type(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Init init)
{
  double w, hz, gain, kbd;
  double const max_res = 0.99;
  int const kbd_pivot = midi_middle_c;
  int this_module = _global ? module_gfx : module_vfx;

  auto const& res_curve = *modulation[this_module][block.module_slot][param_svf_res][0];
  auto const& kbd_curve = *modulation[this_module][block.module_slot][param_svf_kbd][0];
  auto const& freq_curve = *modulation[this_module][block.module_slot][param_svf_freq][0];
  auto const& gain_curve = *modulation[this_module][block.module_slot][param_svf_gain][0];
  int kbd_current = _global ? (block.state.last_midi_note == -1 ? midi_middle_c : block.state.last_midi_note) : block.voice->state.id.key;

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    hz = block.normalized_to_raw_fast<domain_type::log>(this_module, param_svf_freq, freq_curve[f]);
    kbd = block.normalized_to_raw_fast<domain_type::linear>(this_module, param_svf_kbd, kbd_curve[f]);
    gain = block.normalized_to_raw_fast<domain_type::linear>(this_module, param_svf_gain, gain_curve[f]);
    hz *= std::pow(2.0, (kbd_current - kbd_pivot) / 12.0 * kbd);
    hz = std::clamp(hz, flt_min_freq, flt_max_freq);
    w = pi64 * hz / block.sample_rate;
    init(w, res_curve[f] * max_res, gain);
    for (int c = 0; c < 2; c++)
      block.state.own_audio[0][0][c][f] = _svf.next(c, audio_in[c][f]);
  }
}

void
fx_engine::process_delay(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int dly_type = block_auto[param_dly_type][0].step();
  switch (dly_type)
  {
  case dly_type_fdbk_sync:
  case dly_type_fdbk_time:
    process_dly_fdbk(block, audio_in, modulation);
    break;
  case dly_type_multi_sync:
  case dly_type_multi_time:
    process_dly_multi(block, audio_in, modulation);
    break;
  default:
    assert(false); 
    break;
  }
}

void
fx_engine::process_dly_fdbk(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  float const max_feedback = 0.99f;
  auto const& block_auto = block.state.own_block_automation;
  int dly_type = block_auto[param_dly_type][0].step();
  bool sync = dly_is_sync(dly_type);

  auto& l_time_curve = block.state.own_scratch[scratch_dly_fdbk_l];
  auto& r_time_curve = block.state.own_scratch[scratch_dly_fdbk_r];
  auto const& amt_curve = *modulation[module_gfx][block.module_slot][param_dly_amt][0];
  auto const& mix_curve = *modulation[module_gfx][block.module_slot][param_dly_mix][0];
  auto const& spread_curve = *modulation[module_gfx][block.module_slot][param_dly_sprd][0];
  if (sync)
  {
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      l_time_curve[f] = get_timesig_time_value(block, block.state.smooth_bpm[f], module_gfx, param_dly_fdbk_tempo_l);
      r_time_curve[f] = get_timesig_time_value(block, block.state.smooth_bpm[f], module_gfx, param_dly_fdbk_tempo_r);
    }
  }
  else
  {
    float l_time = block_auto[param_dly_fdbk_time_l][0].real();
    float r_time = block_auto[param_dly_fdbk_time_r][0].real();
    std::fill(l_time_curve.begin() + block.start_frame, l_time_curve.begin() + block.end_frame, l_time);
    std::fill(r_time_curve.begin() + block.start_frame, r_time_curve.begin() + block.end_frame, r_time);
  }

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float l_time_samples_t = l_time_curve[f] * block.sample_rate;
    float r_time_samples_t = r_time_curve[f] * block.sample_rate;
    float l_time_t = l_time_samples_t - (int)l_time_samples_t;
    float r_time_t = r_time_samples_t - (int)r_time_samples_t;
    int l_time_samples_0 = (int)l_time_samples_t;
    int l_time_samples_1 = (int)l_time_samples_t + 1;
    int r_time_samples_0 = (int)r_time_samples_t;
    int r_time_samples_1 = (int)r_time_samples_t + 1;

    // + 2*samples in case samples > capacity
    float dry_l = audio_in[0][f];
    float dry_r = audio_in[1][f];
    float wet_l0 = _dly_buffer[0][(_dly_pos + 2 * _dly_capacity - l_time_samples_0) % _dly_capacity];
    float wet_l1 = _dly_buffer[0][(_dly_pos + 2 * _dly_capacity - l_time_samples_1) % _dly_capacity];
    float wet_l_base = ((1 - l_time_t) * wet_l0 + l_time_t * wet_l1) * amt_curve[f] * max_feedback;
    float wet_r0 = _dly_buffer[1][(_dly_pos + 2 * _dly_capacity - r_time_samples_0) % _dly_capacity];
    float wet_r1 = _dly_buffer[1][(_dly_pos + 2 * _dly_capacity - r_time_samples_1) % _dly_capacity];
    float wet_r_base = ((1 - r_time_t) * wet_r0 + r_time_t * wet_r1) * amt_curve[f] * max_feedback;
    _dly_buffer[0][_dly_pos] = dry_l + wet_l_base;
    _dly_buffer[1][_dly_pos] = dry_r + wet_r_base;

    // note: treat spread as unipolar for feedback
    float wet_l = wet_l_base + (1.0f - spread_curve[f]) * wet_r_base;
    float wet_r = wet_r_base + (1.0f - spread_curve[f]) * wet_l_base;
    block.state.own_audio[0][0][0][f] = (1.0f - mix_curve[f]) * dry_l + mix_curve[f] * wet_l;
    block.state.own_audio[0][0][1][f] = (1.0f - mix_curve[f]) * dry_r + mix_curve[f] * wet_r;
    _dly_pos = (_dly_pos + 1) % _dly_capacity;
  }
}

void
fx_engine::process_dly_multi(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int dly_type = block_auto[param_dly_type][0].step();
  int tap_count = block_auto[param_dly_multi_taps][0].step();
  bool sync = dly_is_sync(dly_type);

  auto& time_curve = block.state.own_scratch[scratch_dly_multi_time];
  auto& hold_curve = block.state.own_scratch[scratch_dly_multi_hold];
  auto const& amt_curve = *modulation[module_gfx][block.module_slot][param_dly_amt][0];
  auto const& mix_curve = *modulation[module_gfx][block.module_slot][param_dly_mix][0];
  auto const& spread_curve = *modulation[module_gfx][block.module_slot][param_dly_sprd][0];
  if (sync)
  {
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      hold_curve[f] = get_timesig_time_value(block, block.state.smooth_bpm[f], module_gfx, param_dly_hold_tempo);
      time_curve[f] = get_timesig_time_value(block, block.state.smooth_bpm[f], module_gfx, param_dly_multi_tempo);
    }
  }
  else
  {
    float hold = block_auto[param_dly_hold_time][0].real();
    float time = block_auto[param_dly_multi_time][0].real();
    std::fill(hold_curve.begin() + block.start_frame, hold_curve.begin() + block.end_frame, hold);
    std::fill(time_curve.begin() + block.start_frame, time_curve.begin() + block.end_frame, time);
  }

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float time_samples_t = time_curve[f] * block.sample_rate;
    float hold_samples_t = hold_curve[f] * block.sample_rate;
    float spread = block.normalized_to_raw_fast<domain_type::linear>(module_gfx, param_dly_sprd, spread_curve[f]);

    for (int c = 0; c < 2; c++)
    {
      float wet = 0.0f;
      float tap_amt = 1.0f - (1.0f - amt_curve[f]) * (1.0f - amt_curve[f]);
      for (int t = 0; t < tap_count; t++)
      {
        int lr = (t + c) % 2;
        float tap_bal = stereo_balance(lr, spread);
        float tap_samples_t = (t + 1) * time_samples_t + hold_samples_t;
        float tap_t = tap_samples_t - (int)tap_samples_t;
        int tap_samples_0 = (int)tap_samples_t % _dly_capacity;
        int tap_samples_1 = ((int)tap_samples_t + 1) % _dly_capacity;
        float buffer_sample_0 = _dly_buffer[c][(_dly_pos + 2 * _dly_capacity - tap_samples_0) % _dly_capacity];
        float buffer_sample_1 = _dly_buffer[c][(_dly_pos + 2 * _dly_capacity - tap_samples_1) % _dly_capacity];
        float buffer_sample = ((1 - tap_t) * buffer_sample_0 + tap_t * buffer_sample_1);
        wet += tap_bal * tap_amt * buffer_sample;
        tap_amt *= tap_amt;
      }
      float dry = audio_in[c][f];
      _dly_buffer[c][_dly_pos] = dry;
      block.state.own_audio[0][0][c][f] = (1.0f - mix_curve[f]) * dry + (mix_curve[f] * wet);
    }
    _dly_pos = (_dly_pos + 1) % _dly_capacity;
  }
}

void  
fx_engine::dist_svf_next(plugin_block const& block, int oversmp_factor,
  double freq_plain, double res, float& left, float& right)
{
  double const max_res = 0.99;
  int this_module = _global ? module_gfx : module_vfx;
  double hz = block.normalized_to_raw_fast<domain_type::log>(this_module, param_svf_freq, freq_plain);
  double w = pi64 * hz / (block.sample_rate * oversmp_factor);
  _dst_svf.init_lpf(w, res * max_res);
  left = _dst_svf.next(0, left);
  right = _dst_svf.next(1, right);
}

template <bool Graph, int Type> void
fx_engine::process_dist(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  switch (block_auto[param_dist_clip][0].step())
  {
  case dist_clip_clip: process_dist_clip<Graph, Type>(block, audio_in, modulation, [](float in) { return std::clamp(in, -1.0f, 1.0f); }); break;
  case dist_clip_tanh: process_dist_clip<Graph, Type>(block, audio_in, modulation, [](float in) { return std::tanh(in); }); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Type, class Clip> void
fx_engine::process_dist_clip(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip)
{
  switch (_dst_shape_items[block.state.own_block_automation[param_dist_shape][0].step()].index1)
  {
  case wave_shape_type_saw: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_saw); break;
  case wave_shape_type_sqr: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sqr); break;
  case wave_shape_type_tri: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_tri); break;
  case wave_shape_type_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin); break;
  case wave_shape_type_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos); break;
  case wave_shape_type_sin_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin_sin); break;
  case wave_shape_type_sin_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin_cos); break;
  case wave_shape_type_cos_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos_sin); break;
  case wave_shape_type_cos_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos_cos); break;
  case wave_shape_type_sin_sin_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin_sin_sin); break;
  case wave_shape_type_sin_sin_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin_sin_cos); break;
  case wave_shape_type_sin_cos_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin_cos_sin); break;
  case wave_shape_type_sin_cos_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_sin_cos_cos); break;
  case wave_shape_type_cos_sin_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos_sin_sin); break;
  case wave_shape_type_cos_sin_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos_sin_cos); break;
  case wave_shape_type_cos_cos_sin: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos_cos_sin); break;
  case wave_shape_type_cos_cos_cos: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_cos_cos_cos); break;
  case wave_shape_type_smooth_or_fold: process_dist_clip_shape<Graph, Type>(block, audio_in, modulation, clip, wave_shape_bi_fold); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Type, class Clip, class Shape> void
fx_engine::process_dist_clip_shape(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip, Shape shape)
{
  switch (_dst_shape_items[block.state.own_block_automation[param_dist_shape][0].step()].index2)
  {
  case wave_skew_type_off: process_dist_clip_shape_x<Graph, Type>(block, audio_in, modulation, clip, shape, wave_skew_bi_off); break;
  case wave_skew_type_lin: process_dist_clip_shape_x<Graph, Type>(block, audio_in, modulation, clip, shape, wave_skew_bi_lin); break;
  case wave_skew_type_scu: process_dist_clip_shape_x<Graph, Type>(block, audio_in, modulation, clip, shape, wave_skew_bi_scu); break;
  case wave_skew_type_scb: process_dist_clip_shape_x<Graph, Type>(block, audio_in, modulation, clip, shape, wave_skew_bi_scb); break;
  case wave_skew_type_xpu: process_dist_clip_shape_x<Graph, Type>(block, audio_in, modulation, clip, shape, wave_skew_bi_xpu); break;
  case wave_skew_type_xpb: process_dist_clip_shape_x<Graph, Type>(block, audio_in, modulation, clip, shape, wave_skew_bi_xpb); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Type, class Clip, class Shape, class SkewX> void
fx_engine::process_dist_clip_shape_x(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x)
{
  switch (_dst_shape_items[block.state.own_block_automation[param_dist_shape][0].step()].index3)
  {
  case wave_skew_type_off: process_dist_clip_shape_xy<Graph, Type>(block, audio_in, modulation, clip, shape, skew_x, wave_skew_bi_off); break;
  case wave_skew_type_lin: process_dist_clip_shape_xy<Graph, Type>(block, audio_in, modulation, clip, shape, skew_x, wave_skew_bi_lin); break;
  case wave_skew_type_scu: process_dist_clip_shape_xy<Graph, Type>(block, audio_in, modulation, clip, shape, skew_x, wave_skew_bi_scu); break;
  case wave_skew_type_scb: process_dist_clip_shape_xy<Graph, Type>(block, audio_in, modulation, clip, shape, skew_x, wave_skew_bi_scb); break;
  case wave_skew_type_xpu: process_dist_clip_shape_xy<Graph, Type>(block, audio_in, modulation, clip, shape, skew_x, wave_skew_bi_xpu); break;
  case wave_skew_type_xpb: process_dist_clip_shape_xy<Graph, Type>(block, audio_in, modulation, clip, shape, skew_x, wave_skew_bi_xpb); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Type, class Clip, class Shape, class SkewX, class SkewY> void 
fx_engine::process_dist_clip_shape_xy(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_matrix_mixdown const& modulation, Clip clip, Shape shape, SkewX skew_x, SkewY skew_y)
{
  int this_module = _global ? module_gfx : module_vfx;
  auto const& block_auto = block.state.own_block_automation;
  int dist_shape = block_auto[param_dist_shape][0].step();
  int oversmp_stages = block_auto[param_dist_over][0].step();
  int oversmp_factor = 1 << oversmp_stages;

  auto const& type_item = _dst_shape_items[dist_shape];
  int sx = type_item.index2;
  int sy = type_item.index3;

  auto const& mix_curve = *modulation[this_module][block.module_slot][param_dist_mix][0];
  auto const& res_curve = *modulation[this_module][block.module_slot][param_dist_lp_res][0];
  auto const& x_curve_plain = *modulation[this_module][block.module_slot][param_dist_x][0];
  auto const& y_curve_plain = *modulation[this_module][block.module_slot][param_dist_y][0];
  auto const& gain_curve_plain = *modulation[this_module][block.module_slot][param_dist_gain][0];
  auto const& freq_curve_plain = *modulation[this_module][block.module_slot][param_dist_lp_frq][0];

  jarray<float, 1> const* x_curve = &x_curve_plain;
  if(wave_skew_is_exp(sx))
  {
    auto& x_scratch = block.state.own_scratch[scratch_dist_x];
    for (int f = block.start_frame; f < block.end_frame; f++)
      x_scratch[f] = std::log(0.001 + (x_curve_plain[f] * 0.98)) / log_half;
    x_curve = &x_scratch;
  }

  jarray<float, 1> const* y_curve = &y_curve_plain;
  if (wave_skew_is_exp(sy))
  {
    auto& y_scratch = block.state.own_scratch[scratch_dist_y];
    for (int f = block.start_frame; f < block.end_frame; f++)
      y_scratch[f] = std::log(0.001 + (y_curve_plain[f] * 0.98)) / log_half;
    y_curve = &y_scratch;
  }

  auto& gain_curve = block.state.own_scratch[scratch_dist_gain_raw];
  normalized_to_raw_into_fast<domain_type::log>(block, this_module, param_dist_gain, gain_curve_plain, gain_curve);

  // dont oversample for graphs
  if constexpr(Graph) 
  {
    oversmp_stages = 0;
    oversmp_factor = 1;
  }

  // oversampling is destructive
  audio_in[0].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][0]);
  audio_in[1].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][1]);

  std::array<jarray<float, 2>*, 1> lanes;
  lanes[0] = &block.state.own_audio[0][0];
  _dst_oversampler.process(oversmp_stages, lanes, 1, block.start_frame, block.end_frame,
    [this, &block, &x_curve, &y_curve, &mix_curve, &gain_curve, &freq_curve_plain, &res_curve, 
      this_module, clip, shape, skew_x, skew_y, oversmp_factor](float** lanes_channels, int frame) 
    { 
      float left_in = lanes_channels[0][frame];
      float right_in = lanes_channels[1][frame];
      float& left = lanes_channels[0][frame];
      float& right = lanes_channels[1][frame];

      int mod_index = frame / oversmp_factor;
      left = skew_x(left * gain_curve[mod_index], (*x_curve)[mod_index]);
      right = skew_x(right * gain_curve[mod_index], (*x_curve)[mod_index]);
      if constexpr(Type == type_dst_b)
        dist_svf_next(block, oversmp_factor, freq_curve_plain[mod_index], res_curve[mod_index], left, right);
      left = shape(left);
      right = shape(right);
      if constexpr (Type == type_dst_c)
        dist_svf_next(block, oversmp_factor, freq_curve_plain[mod_index], res_curve[mod_index], left, right);
      left = clip(skew_y(left, (*y_curve)[mod_index]));
      right = clip(skew_y(right, (*y_curve)[mod_index]));
      left = (1 - mix_curve[mod_index]) * left_in + mix_curve[mod_index] * left;
      right = (1 - mix_curve[mod_index]) * right_in + mix_curve[mod_index] * right;
    });
  
  // dont dc filter for graphs
  // since this falls outside of the clip function it may produce stuff outside [-1, 1] 
  if constexpr (!Graph) 
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
      {
        float mixed = block.state.own_audio[0][0][c][f];
        block.state.own_audio[0][0][c][f] = _dst_dc.next(c, mixed);
      }
}

}
