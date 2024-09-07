#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/oversampler.hpp>
#include <plugin_base/dsp/graph_engine.hpp>
#include <plugin_base/shared/io_plugin.hpp>

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
static double const flt_max_res = 0.99;
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

static int const meq_flt_count = 5;
static int const reverb_comb_count = 8;
static int const reverb_allpass_count = 4;
static float const reverb_allpass_length[reverb_allpass_count] = {
  556.0f / 44100.0f, 441.0f / 44100.0f, 341.0f / 44100.0f, 225.0f / 44100.0f };
static float const reverb_comb_length[reverb_comb_count] = {
  1116.0f / 44100.0f, 1188.0f / 44100.0f, 1277.0f / 44100.0f, 1356.0f / 44100.0f,
  1422.0f / 44100.0f, 1491.0f / 44100.0f, 1557.0f / 44100.0f, 1617.0f / 44100.0f };

enum { dly_mode_fdbk, dly_mode_multi };
enum { meq_mode_serial, meq_mode_parallel };
enum { dist_mode_no_filter, dist_mode_filt_to_shape, dist_mode_shape_to_filt };
enum { dist_over_1, dist_over_2, dist_over_4 };
enum { comb_mode_feedforward, comb_mode_feedback, comb_mode_both };
enum { type_off, type_svf, type_cmb, type_dst, type_dsf_dst, type_meq, type_delay, type_reverb };
enum { dist_clip_hard, dist_clip_tanh, dist_clip_sin, dist_clip_exp, dist_clip_tsq, dist_clip_cube, dist_clip_inv };
enum { svf_mode_lpf, svf_mode_hpf, svf_mode_bpf, svf_mode_bsf, svf_mode_apf, svf_mode_peq, svf_mode_bll, svf_mode_lsh, svf_mode_hsh };
enum { meq_flt_mode_off, meq_flt_mode_lpf, meq_flt_mode_hpf, meq_flt_mode_bpf, meq_flt_mode_bsf, meq_flt_mode_apf, meq_flt_mode_peq, meq_flt_mode_bll, meq_flt_mode_lsh, meq_flt_mode_hsh };
enum { section_main, section_svf_left, section_svf_right, section_comb_left, section_comb_right, section_dist_flt, section_dist_mid, 
  section_dist_right, section_meq, section_delay_sync, section_delay_left, section_delay_right, section_reverb_left, section_reverb_right };

enum { scratch_dly_fdbk_l, scratch_dly_fdbk_r, scratch_dly_fdbk_count };
enum { scratch_reverb_damp, scratch_reverb_size, scratch_reverb_in, scratch_reverb_count };
enum { scratch_dly_multi_hold, scratch_dly_multi_time, scratch_dly_multi_sprd, scratch_dly_multi_count };
enum { scratch_dist_x, scratch_dist_y, scratch_dist_gain_raw, scratch_dist_svf_freq, scratch_dist_clip_exp, scratch_dist_count };
enum { scratch_flt_stvar_freq, scratch_flt_stvar_kbd, scratch_flt_stvar_gain, scratch_flt_stvar_count };
enum { scratch_flt_comb_dly_plus, scratch_flt_comb_gain_plus, scratch_flt_comb_dly_min, scratch_flt_comb_gain_min, scratch_flt_comb_gain_count };
enum { scratch_meq_freq, scratch_meq_gain = meq_flt_count, scratch_meq_audio_l = meq_flt_count * 2, scratch_meq_audio_r, scratch_meq_count };
static int constexpr scratch_count = std::max({ 
  (int)scratch_dly_fdbk_count, (int)scratch_dly_multi_count, (int)scratch_dist_count, (int)scratch_meq_count,
  (int)scratch_reverb_count, (int)scratch_flt_comb_gain_count, (int)scratch_flt_stvar_count });

enum { param_type,
  param_svf_mode, param_svf_kbd, param_svf_gain, param_svf_freq, param_svf_res,
  param_comb_mode, param_comb_dly_plus, param_comb_dly_min, param_comb_gain_plus, param_comb_gain_min,
  param_dist_mode, param_dist_lp_frq, param_dist_lp_res, 
  param_dist_gain, param_dist_mix, param_dist_skew_x, param_dist_skew_x_amt, param_dist_skew_y, param_dist_skew_y_amt,  
  param_dist_over, param_dist_clip, param_dist_clip_exp, param_dist_shaper, param_dist_clamp,
  param_dist_dsf_parts, param_dist_dsf_dist, param_dist_dsf_dcy, param_dist_dsf_freq,
  param_meq_mode, param_meq_flt_mode, param_meq_gain, param_meq_freq, param_meq_res,
  param_dly_mode, param_dly_sync, param_dly_amt, param_dly_mix, param_dly_sprd, param_dly_hold_time, param_dly_hold_tempo,
  param_dly_fdbk_time_l, param_dly_fdbk_tempo_l, param_dly_fdbk_time_r, param_dly_fdbk_tempo_r,
  param_dly_multi_taps, param_dly_multi_time, param_dly_multi_tempo,
  param_reverb_mix, param_reverb_spread, param_reverb_apf, param_reverb_size, param_reverb_damp
};

static bool svf_has_gain(int svf_mode) { return svf_mode >= svf_mode_bll; }
static bool type_is_dst(int type) { return type == type_dst || type == type_dsf_dst; }
static constexpr bool meq_has_gain(int meq_flt_mode) { return meq_flt_mode >= meq_flt_mode_bll; }
static bool comb_has_feedback(int comb_mode) { return comb_mode == comb_mode_feedback || comb_mode == comb_mode_both; }
static bool comb_has_feedforward(int comb_mode) { return comb_mode == comb_mode_feedforward || comb_mode == comb_mode_both; }

static std::vector<list_item>
type_items(bool global)
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  result.emplace_back("{9CB55AC0-48CB-43ED-B81E-B97C08771815}", "SV Filter");
  result.emplace_back("{8140F8BC-E4FD-48A1-B147-CD63E9616450}", "Comb Filter");
  result.emplace_back("{277BDD6B-C1F8-4C33-90DB-F4E144FE06A6}", "Distortion");
  result.emplace_back("{006D70FA-F374-440F-9B29-1B693F80DB56}", "DSF Distortion");
  result.emplace_back("{FED71DAA-343D-4B50-8891-B0474901D109}", "Multi EQ");
  if(!global) return result;
  result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  result.emplace_back("{7BB990E6-9A61-4C9F-BDAC-77D1CC260017}", "Reverb");
  return result;
}

static std::vector<list_item>
comb_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{D74DBD2D-CAE9-4993-899B-F1F2AFC18444}", "Feedforward");
  result.emplace_back("{1F75F32E-1938-4314-B14F-CE2D16C8D603}", "Feedback");
  result.emplace_back("{2F21C25C-0CC6-486D-97B3-439BC761D4BF}", "Both");
  return result;
}

static std::vector<list_item>
dist_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{4216D379-72FD-4C2D-B594-20C175CF275E}", "No Filter");
  result.emplace_back("{90657FC7-42E9-4D8E-88D1-5380F916A6C1}", "Filter 2 Shape");
  result.emplace_back("{01871DF5-5834-43F9-B95C-09D8102BF985}", "Shape 2 Filter");
  return result;
}

static std::vector<list_item>
svf_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{8B7E5C75-C3F6-4F53-900C-0A75703F5570}", "Low Pass");
  result.emplace_back("{673C872A-F740-431C-8CD3-F577CE984C2D}", "High Pass");
  result.emplace_back("{9AADCB70-4753-400A-B4C3-A68AFA60273E}", "Band Pass");
  result.emplace_back("{3516F76F-1162-4180-AAE6-BA658A6035C6}", "Band Stop");
  result.emplace_back("{57BA905C-9735-4090-9D4D-75F6CD639387}", "All Pass");
  result.emplace_back("{13013D87-0DBA-47D7-BE59-F6B1B65CA118}", "Peaking EQ");
  result.emplace_back("{463BAD99-6E33-4052-B6EF-31D6D781F002}", "Bell");
  result.emplace_back("{0ECA44F9-57AD-44F4-A066-60A166F4BD86}", "Low Shelf");
  result.emplace_back("{D28FA8B1-3D45-4C80-BAA3-C6735FA4A5E2}", "High Shelf");
  return result;
}

static std::vector<list_item>
meq_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{6B5B4553-FE42-43B5-856D-535F8EE57DD0}", "Serial");
  result.emplace_back("{4F21519B-32CA-49C4-8C24-ED4168E03AFC}", "Parallel");
  return result;
}

static std::vector<list_item>
meq_flt_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{D375AAEB-461A-43B4-A09A-83D57C80EEDB}", "Off", "Off");
  result.emplace_back("{59611761-EE2B-47B9-8400-6AA86A4D6B2E}", "LP", "Low Pass");
  result.emplace_back("{B5B8C797-6CA9-4822-B906-3DEECEB75237}", "HP", "High Pass");
  result.emplace_back("{55DB2D11-F20E-4284-A3F7-06A66BF292F1}", "BP", "Band Pass");
  result.emplace_back("{E27BDD78-02A1-495D-9EDE-B3E9ABFE34F6}", "BS", "Band Stop");
  result.emplace_back("{05A53330-ED0A-4E33-95DD-F8A509A4DD7E}", "AP", "All Pass");
  result.emplace_back("{634D6AC1-889D-4128-B1C0-8B734494E47F}", "PQ", "Peaking EQ");
  result.emplace_back("{0D6B841F-CF91-4D2D-9EFD-E12CD4427363}", "BL", "Bell");
  result.emplace_back("{CC57B060-4B05-44F6-AAFA-4CFD43E5CCC7}", "LS", "Low Shelf");
  result.emplace_back("{393EE610-F904-4BAF-A66E-0BB0A39EA5E7}", "HS", "High Shelf");
  return result;
}

static std::vector<list_item>
dly_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{B70FA3DC-7955-434A-87A6-41CEF9F6CEC6}", "Feedback");
  result.emplace_back("{4E2F10EE-D7F2-44A7-8AF2-A11FB417CB5F}", "Multi Tap");
  return result;
}

static std::vector<list_item>
dist_clip_items(bool clamp)
{
  std::vector<list_item> result;
  result.emplace_back("{FAE2F1EB-248D-4BA2-A008-07C2CD56EB71}", "Hard");
  result.emplace_back("{E40AE5EA-2E84-436F-960E-2D8733F0AA42}", "Tanh");
  result.emplace_back("{BDBE513E-C0B4-4957-9FCC-00C3ED6D116E}", "Sin");
  result.emplace_back("{9F11465A-C46B-4157-BF6A-0135B3A530FA}", "TSQ");
  result.emplace_back("{1D08261B-0980-41B5-955E-D0E093B6D5DA}", "Cube");
  result.emplace_back("{0CC74F43-2741-4A60-B931-2F9B900A5BBA}", "Inv");
  if (clamp) return result;
  result.emplace_back("{9D372B7F-F63F-418A-8FAC-48285E67D8D1}", "Exp");
  return result;
}

static std::vector<list_item>
dist_over_items()
{
  std::vector<list_item> result;
  result.emplace_back("{AFE72C25-18F2-4DB5-A3F0-1A188032F6FB}", "1X");
  result.emplace_back("{0E961515-1089-4E65-99C0-3A493253CF07}", "2X");
  result.emplace_back("{59C0B496-3241-4D56-BE1F-D7B4B08DB64D}", "4X");
  return result;
}

class fx_state_converter:
public state_converter
{
  bool const _global;
  plugin_desc const* const _desc;
public:
  fx_state_converter(plugin_desc const* const desc, bool global): _desc(desc), _global(global) {}
  void post_process_always(load_handler const& handler, plugin_state& new_state) override {}
  void post_process_existing(load_handler const& handler, plugin_state& new_state) override;

  bool handle_invalid_param_value(
    std::string const& new_module_id, int new_module_slot,
    std::string const& new_param_id, int new_param_slot,
    std::string const& old_value, load_handler const& handler, 
    plain_value& new_value) override;
};

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

  // multi-band eq
  std::array<state_var_filter, meq_flt_count> _meq_filters;

  // reverb
  // https://github.com/sinshu/freeverb
  std::array<std::array<float, reverb_comb_count>, 2> _rev_comb_filter = {};
  std::array<std::array<std::int32_t, reverb_comb_count>, 2> _rev_comb_pos = {};
  std::array<std::array<std::vector<float>, reverb_comb_count>, 2> _rev_comb = {};
  std::array<std::array<std::int32_t, reverb_allpass_count>, 2> _rev_allpass_pos = {};
  std::array<std::array<std::vector<float>, reverb_allpass_count>, 2> _rev_allpass = {};

  void process_reverb(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);

  void process_delay(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool Sync>
  void process_delay_sync(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool Sync>
  void process_dly_fdbk_sync(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool Sync>
  void process_dly_multi_sync(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);

  void process_comb(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool Feedforward, bool Feedback>
  void process_comb_mode(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);

  // https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
  void process_svf(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool GlobalUnison>
  void process_svf_uni(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool GlobalUnison, class Init>
  void process_svf_uni_mode(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, Init init);
  
  template <bool Graph>
  void process_dist(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool Graph, int Mode>
  void process_dist_mode(plugin_block& block, 
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <bool Graph, int Mode, class SkewX>
  void process_dist_mode_x(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x);
  template <bool Graph, int Mode, class SkewX, class SkewY>
  void process_dist_mode_xy(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x, SkewY skew_y);
  template <bool Graph, int Mode, class SkewX, class SkewY, bool ClipIsExp, class Clip>
  void process_dist_mode_xy_clip(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x, SkewY skew_y, Clip clip);
  template <bool Graph, int Mode, class SkewX, class SkewY, bool ClipIsExp, class Clip, class Shape>
  void process_dist_mode_xy_clip_shape(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x, SkewY skew_y, Clip clip, Shape shape);
  void dist_svf_next(plugin_block const& block, int oversmp_factor, double freq_hz, double res, float& left, float& right);

  // https://cytomic.com/files/dsp/SvfLinearTrapOptimised2.pdf
  // cannot template out all the filter types because 10^5 is slow compile times
  void process_meq(plugin_block& block,
    jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation);
  template <int N, int MeqFltMode>
  void process_meq_single_filter(
    plugin_block& block, cv_audio_matrix_mixdown const& modulation,
    jarray<float, 1> const& audio_in_l, jarray<float, 1> const& audio_in_r,
    jarray<float, 1>& audio_out_l, jarray<float, 1>& audio_out_r);

public:
  PB_PREVENT_ACCIDENTAL_COPY(fx_engine);
  fx_engine(bool global, int sample_rate, int max_frame_count);

  void reset(plugin_block const*) override;
  void process(plugin_block& block) override { process<false>(block, nullptr, nullptr); }
  template<bool Graph> 
  void process(plugin_block& block, cv_audio_matrix_mixdown const* modulation, jarray<float, 2> const* audio_in);
};

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vfx, 0, param_type, 0, "SV Filter");
  state.set_text_at(module_vfx, 0, param_svf_mode, 0, "Low Pass");
  state.set_text_at(module_vfx, 0, param_svf_res, 0, "50");
  state.set_text_at(module_vfx, 0, param_svf_freq, 0, "20");
}

static void
init_global_default(plugin_state& state)
{
  bool is_fx = state.desc().plugin->type == plugin_type::fx;
  state.set_text_at(module_gfx, 0, param_type, 0, "SV Filter");
  state.set_text_at(module_gfx, 0, param_svf_mode, 0, "Low Pass");
  state.set_text_at(module_gfx, is_fx ? 0: 1, param_type, 0, "Delay");
  state.set_text_at(module_gfx, is_fx ? 0 : 1, param_dly_mode, 0, "Feedback");
  state.set_text_at(module_gfx, is_fx ? 0 : 1, param_dly_sync, 0, "On");
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
  plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step();
  if(type == type_off) return graph_data(graph_data_type::off, {});

  int frame_count = -1;
  int sample_rate = -1;
  jarray<float, 2> audio_in;
  int const shp_cycle_count = 5;
  int const shp_cycle_length = 200;

  auto const params = make_graph_engine_params();
  if (type_is_dst(type))
  {
    // need many samples for filters to stabilize
    sample_rate = 48000;
    frame_count = shp_cycle_length * shp_cycle_count;
    audio_in.resize(jarray<int, 1>(2, frame_count));
    for(int i = 0; i < shp_cycle_count; i++)
      for (int j = 0; j < shp_cycle_length; j++)
        audio_in[0][i * shp_cycle_length + j] = audio_in[1][i * shp_cycle_length + j] 
          = unipolar_to_bipolar((float)j / (shp_cycle_length - 1));
  } else if(type == type_svf || type == type_cmb || type == type_meq)
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
    mapping.module_index, mapping.module_slot, [mapping, sample_rate, frame_count, &audio_in](plugin_block& block) {
    bool global = mapping.module_index == module_gfx;
    fx_engine engine(global, sample_rate, frame_count);
    engine.reset(&block);
    cv_audio_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
    engine.process<true>(block, &modulation, &audio_in);
  });
  engine->process_end();

  auto const& audio = block->state.own_audio[0][0];

  // comb / filter plot FR
  if (type == type_cmb || type == type_svf || type == type_meq)
  {
    auto response = fft(audio[0].data());
    if (type == type_cmb)
      return graph_data(jarray<float, 1>(response), false, 1.0f, false, { "24 kHz" });

    // SVF/MEQ remaps over 0.8 just to look pretty
    std::vector<float> response_mapped(log_remap_series_x(response, 0.8f));
    return graph_data(jarray<float, 1>(response_mapped), false, 1.0f, false, { "24 kHz" });
  }
  
  // distortion - pick result of the last cycle (after filters kick in)
  if (type_is_dst(type))
  {
    int last_cycle_start = (shp_cycle_count - 1) * shp_cycle_length;
    std::vector<float> series(audio[0].cbegin() + last_cycle_start, audio[0].cbegin() + frame_count);
    return graph_data(jarray<float, 1>(series), true, 1.0f, false, { "Distortion" });
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
  bool dly_sync = state.get_plain_at(mapping.module_index, mapping.module_slot, param_dly_sync, 0).step() != 0;
  if (type == type_delay && dly_sync)
  {
    float one_bar_length = timesig_to_time(120, { 1, 1 });
    partition = float_to_string(length / one_bar_length, 2) + " Bar";
  }
  bool stroke_with_area = type == type_reverb;
  float stroke = type == type_delay? 1.0f: 0.25f;
  return graph_data(jarray<float, 2>(
    std::vector<jarray<float, 1>>({ jarray<float, 1>(left), jarray<float, 1>(right) })), 
    stroke, stroke_with_area, { partition });
}

bool 
fx_state_converter::handle_invalid_param_value(
  std::string const& new_module_id, int new_module_slot,
  std::string const& new_param_id, int new_param_slot,
  std::string const& old_value, load_handler const& handler, 
  plain_value& new_value)
{
  // note: param ids are equal between vfx/gfx, gfx just has more
  if (handler.old_version() < plugin_version{ 1, 2, 0 })
  {
    // mode + sync in delay got split out to separate controls
    if (_global && new_param_id == _desc->plugin->modules[module_gfx].params[param_dly_mode].info.tag.id)
    {
      if (old_value == "{A1481F0B-D6FD-4375-BDF9-C01D2F5C5B79}" || old_value == "{7CEC3D1C-6854-4591-9AD7-BDBA9509EA87}")
      {
        // Feedback
        new_value = _desc->raw_to_plain_at(module_gfx, param_dly_mode, dly_mode_fdbk);
        return true;
      }
      if (old_value == "{871622C7-EC8A-4E3B-A76C-CFDE3467A998}" || old_value == "{62EB5BA9-889A-4C46-8534-12881A4F02D1}")
      {
        // Multi
        new_value = _desc->raw_to_plain_at(module_gfx, param_dly_mode, dly_mode_multi);
        return true;
      }
    }

    // distA/distB/distC in fx type got split out to separate A/B/C control
    if (new_param_id == _desc->plugin->modules[module_gfx].params[param_type].info.tag.id)
    {
      if (old_value == "{6CCE41B3-3A74-4F6A-9AB1-660BF492C8E7}")
      {
        // Distortion B
        new_value = _desc->raw_to_plain_at(module_gfx, param_type, type_dst);
        return true;
      }
      if (old_value == "{4A7A2979-0E1F-49E9-87CC-6E82355CFEA7}")
      {
        // Distortion C
        new_value = _desc->raw_to_plain_at(module_gfx, param_type, type_dst);
        return true;
      }
    }

    // Shape+SkewIn/SkewOut got split out to separate shape/skew in/skew out controls
    if (new_param_id == _desc->plugin->modules[module_gfx].params[param_dist_shaper].info.tag.id)
    {
      // format is {guid}-{guid}-{guid}
      if(old_value.size() != 3 * 38 + 2) return false;
      auto shaper_items = wave_shape_type_items(true, _global);
      std::string old_shaper_guid = old_value.substr(0, 38);
      for(int i = 0; i < shaper_items.size(); i++)
        if (old_shaper_guid == shaper_items[i].id)
        {
          new_value = _desc->raw_to_plain_at(module_gfx, param_dist_shaper, i);
          return true;
        }
    }
  }

  // Max distortion oversampling got reduced from 8x to 4x.
  // note: param ids are equal between vfx/gfx, gfx just has more
  if (handler.old_version() < plugin_version{ 1, 7, 2 })
    if (new_param_id == _desc->plugin->modules[module_gfx].params[param_dist_over].info.tag.id)
      if (old_value == "{BAA4877E-1A4A-4D71-8B80-1AC567B7A37B}")
      {
        new_value = _desc->raw_to_plain_at(module_gfx, param_dist_over, dist_over_4);
        return true;
      }

  return false;
}
  
void 
fx_state_converter::post_process_existing(load_handler const& handler, plugin_state& new_state)
{
  std::string old_value;
  int this_module = _global ? module_gfx: module_vfx;
  auto const& modules = new_state.desc().plugin->modules;
  std::string module_id = modules[this_module].info.tag.id;
  if (handler.old_version() < plugin_version{ 1, 2, 0 })
  {
    auto skew_items = wave_skew_type_items();
    for (int i = 0; i < modules[this_module].info.slot_count; i++)
    {
      // old comb filter was always "both"
      new_state.set_plain_at(this_module, i, param_comb_mode, 0,
        _desc->raw_to_plain_at(this_module, param_comb_mode, comb_mode_both));

      // pick up delay sync from old combined mode + sync
      if (_global && handler.old_param_value(modules[this_module].info.tag.id, i, modules[this_module].params[param_dly_mode].info.tag.id, 0, old_value))
      {
        // Time
        if (old_value == "{A1481F0B-D6FD-4375-BDF9-C01D2F5C5B79}" ||
            old_value == "{871622C7-EC8A-4E3B-A76C-CFDE3467A998}")
          new_state.set_plain_at(this_module, i, param_dly_sync, 0,
            _desc->raw_to_plain_at(this_module, param_dly_sync, 0));
        // Sync
        if (old_value == "{7CEC3D1C-6854-4591-9AD7-BDBA9509EA87}" ||
          old_value == "{62EB5BA9-889A-4C46-8534-12881A4F02D1}")
          new_state.set_plain_at(this_module, i, param_dly_sync, 0,
            _desc->raw_to_plain_at(this_module, param_dly_sync, 1));
      }

      // pick up distortion mode from old combined dstA/dstB/dstC
      if (handler.old_param_value(modules[this_module].info.tag.id, i, modules[this_module].params[param_type].info.tag.id, 0, old_value))
      {
        // Distortion B
        if (old_value == "{6CCE41B3-3A74-4F6A-9AB1-660BF492C8E7}")
          new_state.set_plain_at(this_module, i, param_dist_mode, 0,
            _desc->raw_to_plain_at(this_module, param_dist_mode, dist_mode_filt_to_shape));
        // Distortion C
        if (old_value == "{4A7A2979-0E1F-49E9-87CC-6E82355CFEA7}")
          new_state.set_plain_at(this_module, i, param_dist_mode, 0,
            _desc->raw_to_plain_at(this_module, param_dist_mode, dist_mode_shape_to_filt));
      }

      // pick up skew in/skew out mode from old combined shaper + skew x/y
      if (handler.old_param_value(modules[this_module].info.tag.id, i, modules[this_module].params[param_dist_shaper].info.tag.id, 0, old_value))
      {
        // format is {guid}-{guid}-{guid}
        if (old_value.size() == 3 * 38 + 2)
        {
          std::string old_skew_x_guid = old_value.substr(38 + 1, 38);
          std::string old_skew_y_guid = old_value.substr(2 * 38 + 2, 38);
          for (int j = 0; j < skew_items.size(); j++)
          {
            if (skew_items[j].id == old_skew_x_guid)
              new_state.set_plain_at(this_module, i, param_dist_skew_x, 0,
                _desc->raw_to_plain_at(this_module, param_dist_skew_x, j));
            if (skew_items[j].id == old_skew_y_guid)
              new_state.set_plain_at(this_module, i, param_dist_skew_y, 0,
                _desc->raw_to_plain_at(this_module, param_dist_skew_y, j));
          }
        }
      }
    }
  }
}

module_topo
fx_topo(int section, gui_position const& pos, bool global, bool is_fx)
{
  auto voice_info = make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", true, "Voice FX", "Voice FX", "VFX", module_vfx, 10);
  voice_info.description = "Per-voice FX module with state variable filter, comb filter and distortion.";
  auto global_info = make_topo_info("{31EF3492-FE63-4A59-91DA-C2B4DD4A8891}", true, "Global FX", "Global FX", "GFX", module_gfx, 10);
  global_info.description = "Global FX module with state variable filter, comb filter, distortion, delay and reverb.";
  module_stage stage = global ? module_stage::output : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::audio, scratch_count, {
      make_module_dsp_output(false, make_topo_info_basic("{E7C21225-7ED5-45CC-9417-84A69BECA73C}", "Output", 0, 1)) }),
    make_module_gui(section, pos, { { 1, 1 }, { 32, 12, 35, 55, 8 } })));
  result.gui.tabbed_name = "FX";
  result.gui.is_drag_mod_source = true;
 
  result.graph_engine_factory = make_graph_engine;
  if (global) result.default_initializer = [is_fx](auto& s) { init_global_default(s); };
  if (!global) result.default_initializer = init_voice_default;
  result.graph_renderer = render_graph;
  result.gui.menu_handler_factory = [global](plugin_state* state) {
    return make_audio_routing_menu_handler(state, global); };
  result.engine_factory = [global](auto const&, int sample_rate, int max_frame_count) {
    return std::make_unique<fx_engine>(global, sample_rate, max_frame_count); };
  result.state_converter_factory = [global](auto desc) { return std::make_unique<fx_state_converter>(desc, global); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0, 2, 1 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info_basic("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "Type", param_type, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(type_items(global), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  type.gui.submenu->indices.push_back(type_svf);
  type.gui.submenu->indices.push_back(type_cmb);
  type.gui.submenu->indices.push_back(type_dst);
  type.gui.submenu->indices.push_back(type_dsf_dst);
  type.gui.submenu->indices.push_back(type_meq);
  if (global) type.gui.submenu->indices.push_back(type_delay);
  if (global) type.gui.submenu->indices.push_back(type_reverb);
  type.info.description = "Selects the effect type.";

  auto& svf_mode = result.params.emplace_back(make_param(
    make_topo_info("{784282D2-89DB-4053-9206-E11C01F37754}", true, "SV Filter Mode", "Mode", "SVF Mode", param_svf_mode, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(svf_mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  svf_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf_mode.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf || vs[0] == type_off; });
  svf_mode.info.description = "Selects the state-variable filter mode.";
  auto& svf_left = result.sections.emplace_back(make_param_section(section_svf_left,
    make_topo_tag_basic("{DFA6BD01-8F89-42CB-9D0E-E1902193DD5E}", "SV Filter Left"),
    make_param_section_gui({ 0, 1, 2, 2 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  svf_left.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf_left.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || vs[0] == type_svf; });
  svf_left.gui.merge_with_section = section_svf_right;
  auto& svf_kbd = result.params.emplace_back(make_param(
    make_topo_info("{9EEA6FE0-983E-4EC7-A47F-0DFD79D68BCB}", true, "SV Filter KeyTrk", "KeyTrk", "SVF KeyTrk", param_svf_kbd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-2, 2, global ? 0 : 1, 0, true),
    make_param_gui_single(section_svf_left, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  svf_kbd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf_kbd.info.description = "Controls keyboard tracking with -/+2 octaves.";
  auto& svf_gain = result.params.emplace_back(make_param(
    make_topo_info("{FE108A32-770A-415B-9C85-449ABF6A944C}", true, "SV Filter Shelf Gain", "Gain", "SVF Gain", param_svf_gain, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-24, 24, 0, 1, "dB"),
    make_param_gui_single(section_svf_left, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  svf_gain.gui.bindings.enabled.bind_params({ param_type, param_svf_mode }, [](auto const& vs) { return vs[0] == type_svf && svf_has_gain(vs[1]); });
  svf_gain.info.description = "Controls filter gain for shelving filters.";
  
  auto& svf_right = result.sections.emplace_back(make_param_section(section_svf_right,
    make_topo_tag_basic("{0068DAD1-DE32-43CE-9BFE-1CB5B6B7CA3B}", "SV Filter Left"),
    make_param_section_gui({ 0, 3, 2, 2 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  svf_right.gui.merge_with_section = section_svf_left;
  svf_right.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf_right.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || vs[0] == type_svf; });
  auto& svf_freq = result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", true, "SV Filter Freq", "Freq", "SVF Freq", param_svf_freq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(flt_min_freq, flt_max_freq, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_svf_right, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  svf_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf_freq.info.description = "Controls filter frequency.";
  auto& svf_res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", true, "SV Filter Reso", "Reso", "SVF Reso", param_svf_res, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_svf_right, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  svf_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_svf; });
  svf_res.info.description = "Controls filter resonance.";

  auto& comb_mode = result.params.emplace_back(make_param(
    make_topo_info("{93E738FC-F0D1-471C-B46E-467C5869BB03}", true, "Comb Filter Mode", "Mode", "Cmb Mode", param_comb_mode, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(comb_mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  comb_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  comb_mode.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  comb_mode.info.description = "Selects the comb filter mode.";

  auto& comb_left = result.sections.emplace_back(make_param_section(section_comb_left,
    make_topo_tag_basic("{54CF060F-3EE7-4F42-921F-612F8EEA8EB0}", "Comb Filter"),
    make_param_section_gui({ 0, 1, 2, 2 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  comb_left.gui.merge_with_section = section_comb_right;
  comb_left.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  auto& comb_dly_plus = result.params.emplace_back(make_param(
    make_topo_info("{097ECBDB-1129-423C-9335-661D612A9945}", true, "Comb Filter Delay+", "Delay+", "Comb Delay+", param_comb_dly_plus, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb_left, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  comb_dly_plus.gui.bindings.enabled.bind_params({ param_type, param_comb_mode }, [](auto const& vs) { return vs[0] == type_cmb && comb_has_feedforward(vs[1]); });
  comb_dly_plus.info.description = "Feed-forward time.";
  auto& comb_dly_min = result.params.emplace_back(make_param(
    make_topo_info("{D4846933-6AED-4979-AA1C-2DD80B68404F}", true, "Comb Filter Delay-", "Delay-", "Comb Delay-", param_comb_dly_min, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(comb_min_ms, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb_left, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  comb_dly_min.gui.bindings.enabled.bind_params({ param_type, param_comb_mode }, [](auto const& vs) { return vs[0] == type_cmb && comb_has_feedback(vs[1]); });
  comb_dly_min.info.description = "Feed-back time.";

  auto& comb_right = result.sections.emplace_back(make_param_section(section_comb_right,
    make_topo_tag_basic("{9B508F7D-1E8D-4775-96EA-6AE752EA6E55}", "Comb Filter"),
    make_param_section_gui({ 0, 3, 2, 2 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  comb_right.gui.merge_with_section = section_comb_left;
  comb_right.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_cmb; });
  auto& comb_gain_plus = result.params.emplace_back(make_param(
    make_topo_info("{3069FB5E-7B17-4FC4-B45F-A9DFA383CAA9}", true, "Comb Filter Gain+", "Gain+", "Comb Gain+", param_comb_gain_plus, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0.5, 0, true),
    make_param_gui_single(section_comb_right, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  comb_gain_plus.gui.bindings.enabled.bind_params({ param_type, param_comb_mode }, [](auto const& vs) { return vs[0] == type_cmb && comb_has_feedforward(vs[1]); });
  comb_gain_plus.info.description = "Feed-forward amount.";
  auto& comb_gain_min = result.params.emplace_back(make_param(
    make_topo_info("{9684165E-897B-4EB7-835D-D5AAF8E61E65}", true, "Comb Filter Gain-", "Gain-", "Comb Gain-", param_comb_gain_min, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_comb_right, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  comb_gain_min.gui.bindings.enabled.bind_params({ param_type, param_comb_mode }, [](auto const& vs) { return vs[0] == type_cmb && comb_has_feedback(vs[1]); });
  comb_gain_min.info.description = "Feed-back amount.";

  auto& dist_mode = result.params.emplace_back(make_param(
    make_topo_info("{D62129D2-9818-4C05-9705-3D6AEAABA636}", true, "Dist Mode", "Mode", "Dst.Mode", param_dist_mode, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_mode.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_mode.info.description = std::string("Affects where the filter is placed.<br/ >") +
    "No Filter: filter is not used, schema is Input => Gain => Skew In => Shape => Skew Out => Clip => Mix.<br/>" +
    "Filter To Shaper: filter before shape, schema is Input => Gain => Skew In => Filter => Shape => Skew Out => Clip => Mix.<br/>" +
    "Shaper To Filter: filter after shape, schema is Input => Gain => Skew In => Shape => Filter => Skew Out => Clip => Mix.";
  
  auto& dist_flt = result.sections.emplace_back(make_param_section(section_dist_flt,
    make_topo_tag_basic("{EEB74521-81AC-418B-901E-8ED8EE472A9E}", "Distortion Filter"),
    make_param_section_gui({ 0, 1, 2, 1 }, { { 1, 1 }, {
      gui_dimension::auto_size_all, gui_dimension::auto_size_all } }, gui_label_edit_cell_split::horizontal)));
  dist_flt.gui.merge_with_section = section_dist_mid;
  dist_flt.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  auto& dist_lp = result.params.emplace_back(make_param(
    make_topo_info("{C82BC20D-2F1E-4001-BCFB-0C8945D1B329}", true, "Dist LPF Freq", "LPF", "Dist LPF", param_dist_lp_frq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(flt_min_freq, flt_max_freq, flt_max_freq, 1000, 0, "Hz"),
    make_param_gui_single(section_dist_flt, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_lp.gui.bindings.enabled.bind_params({ param_type, param_dist_mode }, [](auto const& vs) { return type_is_dst(vs[0]) && vs[1] != dist_mode_no_filter; });
  dist_lp.info.description = "Lowpass filter frequency inside the oversampling stage.";
  auto& dist_res = result.params.emplace_back(make_param(
    make_topo_info("{A9F6D41F-3C99-44DD-AAAA-BDC1FEEFB250}", true, "Dist LPF Reso", "Res", "Dist Reso", param_dist_lp_res, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_dist_flt, gui_edit_type::knob, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_res.gui.bindings.enabled.bind_params({ param_type, param_dist_mode }, [](auto const& vs) { return type_is_dst(vs[0]) && vs[1] != dist_mode_no_filter; });
  dist_res.info.description = "Lowpass filter resonance inside the oversampling stage.";

  auto& dist_mid = result.sections.emplace_back(make_param_section(section_dist_mid,
    make_topo_tag_basic("{9BA32ACF-FA53-450A-A280-F0F76F30F240}", "Distortion Mid"),
    make_param_section_gui({ 0, 2, 2, 1 }, { { 1, 1 }, { 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all } }, gui_label_edit_cell_split::horizontal)));
  dist_mid.gui.merge_with_section = section_dist_flt;
  dist_mid.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  auto& dist_gain = result.params.emplace_back(make_param(
    make_topo_info("{3FC57F28-075F-44A2-8D0D-6908447AE87C}", true, "Dist Gain", "Gain", "Dist Gain", param_dist_gain, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(0.1, 32, 1, 1, 2, ""),
    make_param_gui_single(section_dist_mid, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_gain.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_gain.info.description = std::string("Gain amount to drive the shaper and X/Y parameters. ") +
    "Use an Osc with gain envelope to have the effect of the distortion gradually fall-off.";
  auto& dist_mix = result.params.emplace_back(make_param(
    make_topo_info("{667D9997-5BE1-48C7-9B50-4F178E2D9FE5}", true, "Dist Mix", "Mix", "Dist Mix", param_dist_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1, 0, true),
    make_param_gui_single(section_dist_mid, gui_edit_type::knob, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_mix.info.description = "Dry/wet mix between input and output signal.";
  auto& dist_skew_in = result.params.emplace_back(make_param(
    make_topo_info("{DAF94A21-BCA4-4D49-BEC0-F0D70CE4F118}", true, "Dist Skew X Mode", "X", "Skew X Mode", param_dist_skew_x, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_dist_mid, gui_edit_type::autofit_list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_skew_in.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_skew_in.info.description = "Before-shape skew: off (cpu efficient, so use it if you dont need the extra control), linear, scale unipolar/bipolar and exponential unipolar/bipolar.";
  auto& dist_skew_in_amt = result.params.emplace_back(make_param(
    make_topo_info("{94A94B06-6217-4EF5-8BA1-9F77AE54076B}", true, "Dist Skew X Amt", "Skew X Amt", "Dist Skew X", param_dist_skew_x_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dist_mid, gui_edit_type::knob, { 0, 4 }, make_label_none())));
  dist_skew_in_amt.gui.bindings.enabled.bind_params({ param_type, param_dist_skew_x }, [](auto const& vs) {
    return type_is_dst(vs[0]) && vs[1] != wave_skew_type_off; });
  dist_skew_in_amt.info.description = "Before-shape skew amount.";
  dist_skew_in.gui.alternate_drag_param_id = dist_skew_in_amt.info.tag.id;
  auto& dist_skew_out = result.params.emplace_back(make_param(
    make_topo_info("{BF8BB684-50E5-414D-9DAD-6290330C0C40}", true, "Dist Skew Y Mode", "Y", "Skew Y Mode", param_dist_skew_y, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_skew_type_items(), "Off"),
    make_param_gui_single(section_dist_mid, gui_edit_type::autofit_list, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_skew_out.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_skew_out.info.description = "After-shape skew: off (cpu efficient, so use it if you dont need the extra control), linear, scale unipolar/bipolar and exponential unipolar/bipolar.";
  auto& dist_skew_out_amt = result.params.emplace_back(make_param(
    make_topo_info("{042570BF-6F02-4F91-9805-6C49FE9A3954}", true, "Dist Skew Y Amt", "Skew Y Amt", "Dist Skew Y", param_dist_skew_y_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dist_mid, gui_edit_type::knob, { 1, 4 }, make_label_none())));
  dist_skew_out_amt.gui.bindings.enabled.bind_params({ param_type, param_dist_skew_y }, [](auto const& vs) {
    return type_is_dst(vs[0]) && vs[1] != wave_skew_type_off; });
  dist_skew_out_amt.info.description = "After-shape skew amount.";
  result.params[param_dist_skew_y].gui.alternate_drag_param_id = dist_skew_out_amt.info.tag.id;

  auto& dist_right = result.sections.emplace_back(make_param_section(section_dist_right,
    make_topo_tag_basic("{4FD908CC-0EBA-4ADD-8622-EB95013CD429}", "Distortion Right"),
    make_param_section_gui({ 0, 3, 2, 2 }, { { 1, 1 }, {
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all }}, gui_label_edit_cell_split::horizontal)));
  dist_right.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_right.gui.autofit_row = 1;
  auto& dist_over = result.params.emplace_back(make_param(
    make_topo_info("{99C6E4A8-F90A-41DC-8AC7-4078A6DE0031}", true, "Dist Oversampling", "OvrSmp", "Dist OvrSmp", param_dist_over, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_over_items(), ""),
    make_param_gui_single(section_dist_right, gui_edit_type::autofit_list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_over.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_over.info.description = "Oversampling factor. If you go really crazy with distortion, this might tip the scale from just-not-acceptible to just-acceptible.";
  auto& dist_clip = result.params.emplace_back(make_param(
    make_topo_info("{810325E4-C3AB-48DA-A770-65887DF57845}", true, "Dist Clip Mode", "Clip", "Dist Clip", param_dist_clip, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_clip_items(false), "Tanh"),
    make_param_gui_single(section_dist_right, gui_edit_type::autofit_list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_clip.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return type_is_dst(vs[0]); });
  dist_clip.info.description = "Selects hard clipping (clamp to [-1, 1]) or various soft clipping functions.";
  auto& dist_clip_expo = result.params.emplace_back(make_param(
    make_topo_info("{A0C0BCE3-1BC3-495F-950B-8849C802B4EA}", true, "Dist Clip Exp", "Exp", "Dist Exp", param_dist_clip_exp, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0.1, 10, 1, 1, ""),
    make_param_gui_single(section_dist_right, gui_edit_type::knob, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_clip_expo.gui.bindings.enabled.bind_params({ param_type, param_dist_clip }, [](auto const& vs) { return type_is_dst(vs[0]) && vs[1] == dist_clip_exp; });
  dist_clip_expo.info.description = "Exponential clipper amount.";
  auto& dist_shaper = result.params.emplace_back(make_param(
    make_topo_info("{BFB5A04F-5372-4259-8198-6761BA52ADEB}", true, "Dist Shape", "Shape", "Dist Shape", param_dist_shaper, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(wave_shape_type_items(true, global), ""),
    make_param_gui_single(section_dist_right, gui_edit_type::autofit_list, { 0, 6 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_shaper.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dst; });
  dist_shaper.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dst; });
  dist_shaper.info.description = "Selects waveshaper type: various periodic functions plus foldback distortion.";
  auto& dist_clamp = result.params.emplace_back(make_param(
    make_topo_info("{11CEE5D2-2A7D-494C-879D-9199E11C95BC}", true, "Dist Clamp Mode", "Clamp", "Dist Clamp", param_dist_clamp, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(dist_clip_items(true), "Tanh"),
    make_param_gui_single(section_dist_right, gui_edit_type::autofit_list, { 0, 6 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_clamp.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf_dst; });
  dist_clamp.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf_dst; });
  dist_clamp.info.description = "Used to bring the input signal into [-1, 1] to be used as the DSF distortion phase.";
  auto& dist_dsf_partials = result.params.emplace_back(make_param(
    make_topo_info("{D0867433-93E6-43FF-87F0-2ED248BC978F}", true, "Dist DSF Partials", "Parts", "Dist Parts", param_dist_dsf_parts, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_log(1, 1000, 2, 20, 0, ""), // TODO integer + less max
    make_param_gui_single(section_dist_right, gui_edit_type::knob, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_dsf_partials.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf_dst; });
  dist_dsf_partials.info.description = "Controls the number of partials (overtones).";
  auto& dist_dsf_dist = result.params.emplace_back(make_param(
    make_topo_info("{90720101-17BA-4326-9982-E46D7CD4F83D}", true, "Dist DSF Distance", "Dist", "Dist Dist", param_dist_dsf_dist, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_linear(0.05, 20, 1, 2, ""),
    make_param_gui_single(section_dist_right, gui_edit_type::knob, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_dsf_dist.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf_dst; });
  dist_dsf_dist.info.description = "Controls the frequency distance between the base frequency and subsequent partials.";
  auto& dist_dsf_dcy = result.params.emplace_back(make_param(
    make_topo_info("{7AE31A39-19F2-4F1D-8626-66B76C2BF2D1}", true, "Dist DSF Decay", "Dcy", "Dist Dcy", param_dist_dsf_dcy, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_dist_right, gui_edit_type::knob, { 1, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_dsf_dcy.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_dsf_dst; });
  dist_dsf_dcy.info.description = "Controls the amplitude decay of successive partials.";
  auto& dist_dsf_freq = result.params.emplace_back(make_param(
    make_topo_info("{5F774577-8354-4292-B1DA-EB84AEC979B3}", true, "Dist DSF Freq", "Freq", "Dist Freq", param_dist_dsf_freq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(flt_min_freq, flt_max_freq, flt_max_freq, 1000, 0, "Hz"),
    make_param_gui_single(section_dist_right, gui_edit_type::knob, { 1, 6 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  dist_dsf_freq.gui.bindings.enabled.bind_params({ param_type, param_dist_mode }, [](auto const& vs) { return vs[0] == type_dsf_dst; });
  dist_dsf_freq.info.description = "Controls the base frequency for the DSF waveshaper.";

  auto& meq_mode = result.params.emplace_back(make_param(
    make_topo_info("{178BC16A-0AC1-435F-9972-ADF4E50393CA}", true, "Multi EQ Mode", "Mode", "MEQ Mode", param_meq_mode, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_item(meq_mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  meq_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_meq; });
  meq_mode.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_meq; });
  meq_mode.info.description = "Selects the multi-equalizer mode.";
  auto& meq_section = result.sections.emplace_back(make_param_section(section_meq,
    make_topo_tag_basic("{1DF23465-C416-44DA-A318-0266B5A58217}", "Multi EQ"),
    make_param_section_gui({ 0, 1, 2, 4 }, { { 1, 1 }, { 1, 1, 1, 1 } })));
  meq_section.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_meq; });
  meq_section.gui.wrap_in_container = false;
  auto& meq_flt_mode = result.params.emplace_back(make_param(
    make_topo_info("{AFA9AC65-FC61-4F87-8298-9274C470E159}", true, "Multi EQ Filter Mode", "Mode", "MEQ Filter Mode", param_meq_flt_mode, meq_flt_count),
    make_param_dsp_automate_if_voice(!global), make_domain_item(meq_flt_mode_items(), "Off"),
    make_param_gui(section_meq, gui_edit_type::autofit_list, param_layout::own_grid, { 0, 0, 2, 1 },
      make_label_none())));
  meq_flt_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_meq; });
  meq_flt_mode.info.description = "Multi EQ filter mode.";
  meq_flt_mode.gui.multi_own_grid = { { 1, 1 }, { 1, 1, 1 } };
  meq_flt_mode.gui.multi_own_grid_label = "Mode";
  meq_flt_mode.domain.default_selector_ = [](int, int s) { 
    if (s == 0) return "HP"; 
    if (s == meq_flt_count - 1) return "LP"; 
    return "BL"; };
  auto& meq_gain = result.params.emplace_back(make_param(
    make_topo_info("{C0FCE489-AC0C-408A-8D42-E154FD609E28}", true, "Multi EQ Gain", "Gain", "MEQ Gain", param_meq_gain, meq_flt_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(-24, 24, 12, 1, "dB"),
    make_param_gui(section_meq, gui_edit_type::knob, param_layout::own_grid, { 0, 3, 2, 1 },
      make_label(gui_label_contents::drag, gui_label_align::left, gui_label_justify::near))));
  meq_gain.gui.multi_own_grid = { { 1, 1 }, { 1, 1, 1 } };
  meq_gain.gui.multi_own_grid_label = "Gain";
  meq_gain.gui.bindings.enabled.bind_params({ param_type, param_meq_flt_mode }, [](auto const& vs) { return vs[0] == type_meq && meq_has_gain(vs[1]); });
  meq_gain.info.description = "Controls filter gain for shelving filters.";
  auto& meq_freq = result.params.emplace_back(make_param(
    make_topo_info("{B2F07A92-51EE-4C95-9A81-DF62D2EC3D17}", true, "Multi EQ Freq", "Freq", "MEQ Freq", param_meq_freq, meq_flt_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(flt_min_freq, flt_max_freq, 1000, 1000, 0, "Hz"),
    make_param_gui(section_meq, gui_edit_type::knob, param_layout::own_grid, { 0, 1, 2, 1 },
      make_label(gui_label_contents::drag, gui_label_align::left, gui_label_justify::near))));
  meq_freq.gui.bindings.enabled.bind_params({ param_type, param_meq_flt_mode }, [](auto const& vs) { return vs[0] == type_meq && vs[1] != meq_flt_mode_off; });
  meq_freq.info.description = "Controls filter frequency.";
  meq_freq.gui.multi_own_grid = { { 1, 1 }, { 1, 1, 1 } };
  meq_freq.gui.multi_own_grid_label = "Freq";
  meq_freq.domain.default_selector_ = [](int, int s) { return std::to_string(100 * std::pow(3, s)); };
  meq_freq.domain.default_selector_ = [](int, int s) { 
    if (s == 0) return std::string("50"); 
    if (s == meq_flt_count - 1) return std::string("10000"); 
    return std::to_string(250 * std::pow(4, s - 1)); };
  auto& meq_res = result.params.emplace_back(make_param(
    make_topo_info("{1E344731-7793-46B7-99D0-E7061515FB63}", true, "Multi EQ Reso", "Reso", "MEQ Reso", param_meq_res, meq_flt_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui(section_meq, gui_edit_type::knob, param_layout::own_grid, { 0, 2, 2, 1 },
      make_label(gui_label_contents::drag, gui_label_align::left, gui_label_justify::near))));
  meq_res.gui.bindings.enabled.bind_params({ param_type, param_meq_flt_mode }, [](auto const& vs) { return vs[0] == type_meq && vs[1] != meq_flt_mode_off; });
  meq_res.info.description = "Controls filter resonance.";
  meq_res.gui.multi_own_grid = { { 1, 1 }, { 1, 1, 1 } };
  meq_res.gui.multi_own_grid_label = "Reso";
  meq_res.domain.default_selector_ = [](int, int s) {
    if (s == 0 || s == meq_flt_count - 1) return "50";
    return "90"; };

  // delay lines and reverb global only, per-voice uses too much memory
  if(!global) return result;

  auto& delay_mode = result.params.emplace_back(make_param(
    make_topo_info("{C2E282BA-9E4F-4AE6-A055-8B5456780C66}", true, "Delay Mode", "Mode", "Dly Mode", param_dly_mode, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_item(dly_mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_mode.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_mode.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_mode.info.description = "Selects feedback or multi-tap delay.";
  auto& delay_sync_section = result.sections.emplace_back(make_param_section(section_delay_sync,
    make_topo_tag_basic("{E92225CF-21BF-459C-8C9D-8E50285F26D4}", "Delay Sync"),
    make_param_section_gui({ 0, 1, 2, 1 }, { { 1, 1 }, { 1 } }, gui_label_edit_cell_split::vertical)));
  delay_sync_section.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_sync_section.gui.merge_with_section = section_delay_left;
  auto& delay_sync = result.params.emplace_back(make_param(
    make_topo_info("{50E6B543-9BC2-490A-8CE3-CB80076BD8E1}", true, "Tempo Sync", "Sync", "Sync", param_dly_sync, 1),
    make_param_dsp_automate_if_voice(!global), make_domain_toggle(false),
    make_param_gui_single(section_delay_sync, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::top, gui_label_justify::center))));
  delay_sync.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_sync.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_sync.info.description = "Toggles time or tempo-synced type.";
  
  auto& delay_left = result.sections.emplace_back(make_param_section(section_delay_left,
    make_topo_tag_basic("{B9841FED-30EA-4D9E-97A0-C22F3D0D87A1}", "Delay Left"),
    make_param_section_gui({ 0, 2, 2, 1 }, { { 1, 1 }, {
      gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  delay_left.gui.merge_with_section = section_delay_sync;
  delay_left.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_amt = result.params.emplace_back(make_param(
    make_topo_info("{7CEE3B9A-99CF-46D3-847B-42F91A4F5227}", true, "Delay Amount", "Amount", "Delay Amount", param_dly_amt, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_delay_left, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_amt.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_amt.info.description = "Feedback-amount or tap-amount control.";
  auto& delay_mix = result.params.emplace_back(make_param(
    make_topo_info("{6933B1F7-886F-41F0-8D23-175AA537327E}", true, "Delay Mix", "Mix", "Delay Mix", param_dly_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_delay_left, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_mix.info.description = "Dry/wet control.";

  auto& delay_right = result.sections.emplace_back(make_param_section(section_delay_right,
    make_topo_tag_basic("{DBD2F824-4B1F-4761-B0FB-46DD3C97A29F}", "Delay Right"),
    make_param_section_gui({ 0, 3, 2, 2 }, { { 1, 1 }, {
      gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  delay_right.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_sprd = result.params.emplace_back(make_param(
    make_topo_info("{1BD8008B-DC2C-4A77-A5DE-869983E5786C}", true, "Delay Spread", "Spread", "Delay Spread", param_dly_sprd, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_delay_right, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_sprd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  delay_sprd.info.description = "Stereo spread control.";
  auto& delay_hold_time = result.params.emplace_back(make_param(
    make_topo_info("{037E4A64-8F80-4E0A-88A0-EE1BB83C99C6}", true, "Multi Delay Hold Time", "Hold", "MDly Hold", param_dly_hold_time, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 0, 1, 3, "Sec"),
    make_param_gui_single(section_delay_right, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_hold_time.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[2] == 0; });
  delay_hold_time.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_multi && vs[2] == 0; });
  delay_hold_time.info.description = "Multi-tap delay hold time in seconds.";
  auto& delay_hold_tempo = result.params.emplace_back(make_param(
    make_topo_info("{AED0D3A5-AB02-441F-A42D-7E2AEE88DF24}", true, "Multi Delay Hold Tempo", "Hold", "MDly Hold", param_dly_hold_tempo, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(true, { 4, 1 }, { 0, 1 }),
    make_param_gui_single(section_delay_right, gui_edit_type::list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_hold_tempo.gui.submenu = make_timesig_submenu(delay_hold_tempo.domain.timesigs);
  delay_hold_tempo.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[2] != 0; });
  delay_hold_tempo.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_multi && vs[2] != 0; });
  delay_hold_tempo.info.description = "Multi-tap delay hold time in bars.";

  auto& delay_fdbk_time_l = result.params.emplace_back(make_param(
    make_topo_info("{E32F17BC-03D2-4F2D-8292-2B4C3AB24E8D}", true, "Fdbk Delay Time L", "Left", "FDly L", param_dly_fdbk_time_l, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 1, 1, 3, "Sec"),
    make_param_gui_single(section_delay_right, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_fdbk_time_l.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[1] == dly_mode_fdbk && vs[2] == 0; });
  delay_fdbk_time_l.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_fdbk && vs[2] == 0; });
  delay_fdbk_time_l.info.description = "Feedback delay left length in seconds.";
  auto& delay_fdbk_tempo_l = result.params.emplace_back(make_param(
    make_topo_info("{33BCF50C-C7DE-4630-A835-44D50DA3B8BB}", true, "Fdbk Delay Tempo L", "Left", "FDly L", param_dly_fdbk_tempo_l, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(false, { 4, 1 }, { 3, 16 }),
    make_param_gui_single(section_delay_right, gui_edit_type::list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_fdbk_tempo_l.gui.submenu = make_timesig_submenu(delay_fdbk_tempo_l.domain.timesigs);
  delay_fdbk_tempo_l.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[1] == dly_mode_fdbk && vs[2] != 0; });
  delay_fdbk_tempo_l.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_fdbk && vs[2] != 0; });
  delay_fdbk_tempo_l.info.description = "Feedback delay left length in bars.";
  auto& delay_fdbk_time_r = result.params.emplace_back(make_param(
    make_topo_info("{5561243C-838F-4C33-BD46-3E934E854969}", true, "Fdbk Delay Time R", "Right", "FDly R", param_dly_fdbk_time_r, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 1, 1, 3, "Sec"),
    make_param_gui_single(section_delay_right, gui_edit_type::hslider, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_fdbk_time_r.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[1] == dly_mode_fdbk && vs[2] == 0; });
  delay_fdbk_time_r.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_fdbk && vs[2] == 0; });
  delay_fdbk_time_r.info.description = "Feedback delay right length in seconds.";
  auto& delay_fdbk_tempo_r = result.params.emplace_back(make_param(
    make_topo_info("{4FA78F9E-AC3A-45D7-A8A3-E0E2C7C264D7}", true, "Fdbk Delay Tempo R", "Right", "FDly R", param_dly_fdbk_tempo_r, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(false, { 4, 1 }, { 5, 16 }),
    make_param_gui_single(section_delay_right, gui_edit_type::list, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_fdbk_tempo_r.gui.submenu = make_timesig_submenu(delay_fdbk_tempo_r.domain.timesigs);
  delay_fdbk_tempo_r.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[1] == dly_mode_fdbk && vs[2] != 0; });
  delay_fdbk_tempo_r.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_fdbk && vs[2] != 0; });
  delay_fdbk_tempo_r.info.description = "Feedback delay right length in bars.";
  auto& delay_taps = result.params.emplace_back(make_param(
    make_topo_info("{27572912-0A8E-4A97-9A54-379829E8E794}", true, "Multi Delay Tap Count", "Taps", "MDly Taps", param_dly_multi_taps, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_step(1, 8, 4, 0),
    make_param_gui_single(section_delay_right, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_taps.gui.bindings.visible.bind_params({ param_type, param_dly_mode }, [](auto const& vs) { return vs[1] == dly_mode_multi; });
  delay_taps.gui.bindings.enabled.bind_params({ param_type, param_dly_mode }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_multi; });
  delay_taps.info.description = "Multi-tap delay tap count.";
  auto& delay_multi_time = result.params.emplace_back(make_param(
    make_topo_info("{8D1A0D44-3291-488F-AC86-9B2B608F9562}", true, "Multi Delay Time", "Time", "MDly Time", param_dly_multi_time, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_log(0, dly_max_sec, 1, 1, 3, "Sec"),
    make_param_gui_single(section_delay_right, gui_edit_type::hslider, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_multi_time.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[1] == dly_mode_multi && vs[2] == 0; });
  delay_multi_time.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_multi && vs[2] == 0; });
  delay_multi_time.info.description = "Multi-tap delay length in seconds.";
  auto& delay_multi_tempo = result.params.emplace_back(make_param(
    make_topo_info("{8DAED046-7F5F-4E76-A6BF-099510564500}", true, "Multi Delay Tempo", "Tempo", "MDly Tempo", param_dly_multi_tempo, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_timesig_default(false, { 4, 1 }, { 3, 16 }),
    make_param_gui_single(section_delay_right, gui_edit_type::list, { 1, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  delay_multi_tempo.gui.submenu = make_timesig_submenu(delay_multi_tempo.domain.timesigs);
  delay_multi_tempo.gui.bindings.visible.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[1] == dly_mode_multi && vs[2] != 0; });
  delay_multi_tempo.gui.bindings.enabled.bind_params({ param_type, param_dly_mode, param_dly_sync }, [](auto const& vs) { return vs[0] == type_delay && vs[1] == dly_mode_multi && vs[2] != 0; });
  delay_multi_tempo.info.description = "Multi-tap delay length in bars.";

  auto& reverb_mix = result.params.emplace_back(make_param(
    make_topo_info("{7F71B450-2EAA-4D4E-8919-A94D87645DB0}", true, "Reverb Mix", "Mix", "Reverb Mix", param_reverb_mix, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  reverb_mix.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  reverb_mix.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  reverb_mix.info.description = "Reverb dry/wet control.";  
  auto& reverb_left = result.sections.emplace_back(make_param_section(section_reverb_left,
    make_topo_tag_basic("{92EFDFE7-41C5-4E9D-9BE6-DC56965C1C0D}", "Reverb Left"),
    make_param_section_gui({ 0, 1, 2, 2 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  reverb_left.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_sprd = result.params.emplace_back(make_param(
    make_topo_info("{0D138920-65D2-42E9-98C5-D8FEC5FD2C55}", true, "Reverb Spread", "Spread", "Reverb Spread", param_reverb_spread, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.5, 0, true),
    make_param_gui_single(section_reverb_left, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  reverb_sprd.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  reverb_sprd.info.description = "Reverb stereo-spread control.";
  auto& reverb_apf = result.params.emplace_back(make_param(
    make_topo_info("{09DF58B0-4155-47F2-9AEB-927B2D8FD250}", true, "Reverb APF", "APF", "Reverb APF", param_reverb_apf, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(1.0, 0, true),
    make_param_gui_single(section_reverb_left, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  reverb_apf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  reverb_apf.info.description = "Reverb all-pass filter amount.";

  auto& reverb_right = result.sections.emplace_back(make_param_section(section_reverb_right,
    make_topo_tag_basic("{EB2AE24C-1AF8-49A9-B4CA-B1F974362DD2}", "Reverb Right"),
    make_param_section_gui({ 0, 3, 2, 2 }, { { 1, 1 }, { gui_dimension::auto_size_all, 1 } }, gui_label_edit_cell_split::horizontal)));
  reverb_right.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  auto& reverb_size = result.params.emplace_back(make_param(
    make_topo_info("{E413FA18-420D-4510-80D1-54E2A0ED4CB2}", true, "Reverb Size", "Size", "Reverb Size", param_reverb_size, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.8, 0, true),
    make_param_gui_single(section_reverb_right, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  reverb_size.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  reverb_size.info.description = "Reverb room size.";
  auto& reverb_damp = result.params.emplace_back(make_param(
    make_topo_info("{44EE5538-9920-4F39-A68E-51E86E96943B}", true, "Reverb Damping", "Damp", "Reverb Damp", param_reverb_damp, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0.8, 0, true),
    make_param_gui_single(section_reverb_right, gui_edit_type::hslider, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  reverb_damp.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_reverb; });
  reverb_damp.info.description = "Reverb damping factor.";

  return result;
}

template <int SVFMode>
static void init_svf(state_var_filter& flt, double w, double res, double gn)
{
  if constexpr (SVFMode == svf_mode_lpf) flt.init_lpf(w, res);
  else if constexpr (SVFMode == svf_mode_hpf) flt.init_hpf(w, res);
  else if constexpr (SVFMode == svf_mode_bpf) flt.init_bpf(w, res);
  else if constexpr (SVFMode == svf_mode_bsf) flt.init_bsf(w, res);
  else if constexpr (SVFMode == svf_mode_apf) flt.init_apf(w, res);
  else if constexpr (SVFMode == svf_mode_peq) flt.init_peq(w, res);
  else if constexpr (SVFMode == svf_mode_bll) flt.init_bll(w, res, gn);
  else if constexpr (SVFMode == svf_mode_lsh) flt.init_lsh(w, res, gn);
  else if constexpr (SVFMode == svf_mode_hsh) flt.init_hsh(w, res, gn);
  else assert(false);
}

fx_engine::
fx_engine(bool global, int sample_rate, int max_frame_count) :
_global(global), _dly_capacity(sample_rate * dly_max_sec), 
_dst_oversampler(max_frame_count)
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
  
  for (int i = 0; i < meq_flt_count; i++)
    _meq_filters[i].clear();

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
  cv_audio_matrix_mixdown const* modulation, jarray<float, 2> const* audio_in)
{ 
  if (audio_in == nullptr)
  {
    int this_module = _global ? module_gfx : module_vfx;
    auto& mixer = get_audio_audio_matrix_mixer(block, _global);
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
    modulation = &get_cv_audio_matrix_mixdown(block, _global);

  switch (type)
  {
  case type_meq: process_meq(block, *audio_in, *modulation); break;
  case type_svf: process_svf(block, *audio_in, *modulation); break;
  case type_cmb: process_comb(block, *audio_in, *modulation); break;
  case type_delay: process_delay(block, *audio_in, *modulation); break;
  case type_reverb: process_reverb(block, *audio_in, *modulation); break;
  case type_dst: case type_dsf_dst: process_dist<Graph>(block, *audio_in, *modulation); break;
  default: assert(false); break;
  }
}

void
fx_engine::process_comb(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  int mode = block.state.own_block_automation[param_comb_mode][0].step();
  switch (mode)
  {
  case comb_mode_both: process_comb_mode<true, true>(block, audio_in, modulation); break;
  case comb_mode_feedback: process_comb_mode<false, true>(block, audio_in, modulation); break;
  case comb_mode_feedforward: process_comb_mode<true, false>(block, audio_in, modulation); break;
  default: assert(false); break;
  }
}

template <bool Feedforward, bool Feedback>
void fx_engine::process_comb_mode(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  static_assert(Feedforward || Feedback);
  
  float const feedback_factor = 0.98;
  int this_module = _global ? module_gfx : module_vfx;
  
  auto const& dly_min_curve_norm = *modulation[this_module][block.module_slot][param_comb_dly_min][0];
  auto const& dly_plus_curve_norm = *modulation[this_module][block.module_slot][param_comb_dly_plus][0];
  auto const& gain_min_curve_norm = *modulation[this_module][block.module_slot][param_comb_gain_min][0];
  auto const& gain_plus_curve_norm = *modulation[this_module][block.module_slot][param_comb_gain_plus][0];

  auto& dly_min_curve = block.state.own_scratch[scratch_flt_comb_dly_min];
  auto& dly_plus_curve = block.state.own_scratch[scratch_flt_comb_dly_plus];
  auto& gain_min_curve = block.state.own_scratch[scratch_flt_comb_gain_min];
  auto& gain_plus_curve = block.state.own_scratch[scratch_flt_comb_gain_plus];

  if constexpr (Feedback)
  {
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_comb_dly_min, dly_min_curve_norm, dly_min_curve);
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_comb_gain_min, gain_min_curve_norm, gain_min_curve);
  }

  if constexpr (Feedforward)
  {
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_comb_dly_plus, dly_plus_curve_norm, dly_plus_curve);
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_comb_gain_plus, gain_plus_curve_norm, gain_plus_curve);
  }

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float gain_min = 0;
    float dly_min_t = 0;
    int dly_min_samples_0 = 0;
    int dly_min_samples_1 = 0;

    float gain_plus = 0;
    float dly_plus_t = 0;
    int dly_plus_samples_0 = 0;
    int dly_plus_samples_1 = 0;

    if constexpr (Feedback)
    {
      gain_min = gain_min_curve[f];
      float dly_min = dly_min_curve[f];
      float dly_min_samples_t = dly_min * block.sample_rate * 0.001;
      dly_min_t = dly_min_samples_t - (int)dly_min_samples_t;
      dly_min_samples_0 = (int)dly_min_samples_t;
      dly_min_samples_1 = (int)dly_min_samples_t + 1;
    }

    if constexpr (Feedforward)
    {
      gain_plus = gain_plus_curve[f];
      float dly_plus = dly_plus_curve[f];
      float dly_plus_samples_t = dly_plus * block.sample_rate * 0.001;
      dly_plus_t = dly_plus_samples_t - (int)dly_plus_samples_t;
      dly_plus_samples_0 = (int)dly_plus_samples_t;
      dly_plus_samples_1 = (int)dly_plus_samples_t + 1;
    }

    for (int c = 0; c < 2; c++)
    {
      float min = 0;
      float plus = 0;

      // double mod is needed to not go negative
      if constexpr(Feedback)
      {
        float min0 = _comb_out[c][(((_comb_pos - dly_min_samples_0) % _comb_samples) + _comb_samples) % _comb_samples];
        float min1 = _comb_out[c][(((_comb_pos - dly_min_samples_1) % _comb_samples) + _comb_samples) % _comb_samples];
        min = ((1 - dly_min_t) * min0 + dly_min_t * min1) * gain_min;
      }
      
      if constexpr(Feedforward)
      {
        float plus0 = _comb_in[c][(((_comb_pos - dly_plus_samples_0) % _comb_samples) + _comb_samples) % _comb_samples];
        float plus1 = _comb_in[c][(((_comb_pos - dly_plus_samples_1) % _comb_samples) + _comb_samples) % _comb_samples];
        plus = ((1 - dly_plus_t) * plus0 + dly_plus_t * plus1) * gain_plus;
      }
      
      _comb_in[c][_comb_pos] = audio_in[c][f];
      _comb_out[c][_comb_pos] = audio_in[c][f] + plus + min * feedback_factor;
      block.state.own_audio[0][0][c][f] = _comb_out[c][_comb_pos];
    }
    _comb_pos = (_comb_pos + 1) % _comb_samples;
  }
}

void
fx_engine::process_reverb(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
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
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  if(!_global && block.voice->state.sub_voice_count > 1)
    process_svf_uni<true>(block, audio_in, modulation);
  else
    process_svf_uni<false>(block, audio_in, modulation);
}

template <bool GlobalUnison> void
fx_engine::process_svf_uni(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int svf_mode = block_auto[param_svf_mode][0].step();
  switch (svf_mode)
  {
  case svf_mode_lpf: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_lpf>(_svf, w, res, gn); }); break;
  case svf_mode_hpf: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_hpf>(_svf, w, res, gn); }); break;
  case svf_mode_bpf: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_bpf>(_svf, w, res, gn); }); break;
  case svf_mode_bsf: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_bsf>(_svf, w, res, gn); }); break;
  case svf_mode_apf: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_apf>(_svf, w, res, gn); }); break;
  case svf_mode_peq: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_peq>(_svf, w, res, gn); }); break;
  case svf_mode_bll: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_bll>(_svf, w, res, gn); }); break;
  case svf_mode_lsh: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_lsh>(_svf, w, res, gn); }); break;
  case svf_mode_hsh: process_svf_uni_mode<GlobalUnison>(block, audio_in, modulation, [this](double w, double res, double gn) { init_svf<svf_mode_hsh>(_svf, w, res, gn); }); break;
  default: assert(false); break;
  }
}

template <bool GlobalUnison, class Init> void
fx_engine::process_svf_uni_mode(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, Init init)
{
  double w, hz, gain, kbd;
  int const kbd_pivot = midi_middle_c;
  int this_module = _global ? module_gfx : module_vfx;

  jarray<float, 1> const* glob_uni_dtn_curve = nullptr;
  if constexpr(GlobalUnison)
    glob_uni_dtn_curve = &block.state.all_accurate_automation[module_voice_in][0][voice_in_param_uni_dtn][0];

  auto const& res_curve = *modulation[this_module][block.module_slot][param_svf_res][0];  
  double kbd_trk_base = _global ? (block.state.last_midi_note == -1 ? midi_middle_c : block.state.last_midi_note) : block.voice->state.note_id_.key;

  auto const& kbd_curve_norm = *modulation[this_module][block.module_slot][param_svf_kbd][0];
  auto& kbd_curve = block.state.own_scratch[scratch_flt_stvar_kbd];
  block.normalized_to_raw_block<domain_type::linear>(this_module, param_svf_kbd, kbd_curve_norm, kbd_curve);

  auto const& freq_curve_norm = *modulation[this_module][block.module_slot][param_svf_freq][0];
  auto& freq_curve = block.state.own_scratch[scratch_flt_stvar_freq];
  block.normalized_to_raw_block<domain_type::log>(this_module, param_svf_freq, freq_curve_norm, freq_curve);

  auto const& gain_curve_norm = *modulation[this_module][block.module_slot][param_svf_gain][0];
  auto& gain_curve = block.state.own_scratch[scratch_flt_stvar_gain];
  block.normalized_to_raw_block<domain_type::linear>(this_module, param_svf_gain, gain_curve_norm, gain_curve);

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    hz = freq_curve[f];
    kbd = kbd_curve[f];
    gain = gain_curve[f];

    // correct keyboard tracking for global unison
    double kbd_trk = kbd_trk_base;
    if constexpr(GlobalUnison)
    {
      float voice_pos = (float)block.voice->state.sub_voice_index / (block.voice->state.sub_voice_count - 1.0f);
      kbd_trk += (voice_pos - 0.5f) * (*glob_uni_dtn_curve)[f];
    }

    hz *= std::pow(2.0, (kbd_trk - kbd_pivot) / 12.0 * kbd);
    hz = std::clamp(hz, flt_min_freq, flt_max_freq);
    w = pi64 * hz / block.sample_rate;
    init(w, res_curve[f] * flt_max_res, gain);
    for (int c = 0; c < 2; c++)
      block.state.own_audio[0][0][c][f] = _svf.next(c, audio_in[c][f]);
  }
}

template <int N, int MeqFltMode>
void fx_engine::process_meq_single_filter(
  plugin_block& block, cv_audio_matrix_mixdown const& modulation,
  jarray<float, 1> const& audio_in_l, jarray<float, 1> const& audio_in_r,
  jarray<float, 1>& audio_out_l, jarray<float, 1>& audio_out_r)
{
  static_assert(MeqFltMode != meq_flt_mode_off);
  int this_module = _global ? module_gfx : module_vfx;
  jarray<float, 1> const& res_curve = *modulation[this_module][block.module_slot][param_meq_res][N];

  auto const& freq_curve_norm = *modulation[this_module][block.module_slot][param_meq_freq][N];
  auto& freq_curve = block.state.own_scratch[scratch_meq_freq + N];
  block.normalized_to_raw_block<domain_type::log>(this_module, param_meq_freq, freq_curve_norm, freq_curve);

  auto const& gain_curve_norm = *modulation[this_module][block.module_slot][param_meq_gain][N];
  auto& gain_curve = block.state.own_scratch[scratch_meq_gain + N];
  if constexpr (meq_has_gain(MeqFltMode))
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_meq_gain, gain_curve_norm, gain_curve);

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    double res = res_curve[f];
    double gain = gain_curve[f];
    double hz = std::clamp((double)freq_curve[f], flt_min_freq, flt_max_freq);
    double w = pi64 * hz / block.sample_rate;
    init_svf<MeqFltMode - 1>(_meq_filters[N], w, res * flt_max_res, gain);
    audio_out_l[f] = _meq_filters[N].next(0, audio_in_l[f]);
    audio_out_r[f] = _meq_filters[N].next(1, audio_in_r[f]);
  }
}

void
fx_engine::process_meq(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  int active_filters = 5;
  auto const& block_auto = block.state.own_block_automation;
  bool is_parallel = block_auto[param_meq_mode][0].step() == meq_mode_parallel;

  jarray<float, 1> const* audio_in_l = &block.state.own_audio[0][0][0];
  jarray<float, 1> const* audio_in_r = &block.state.own_audio[0][0][1];
  jarray<float, 1>* audio_out_l = &block.state.own_audio[0][0][0];
  jarray<float, 1>* audio_out_r = &block.state.own_audio[0][0][1];

  if (is_parallel)
  {
    audio_in_l = &audio_in[0];
    audio_in_r = &audio_in[1];
    audio_out_l = &block.state.own_scratch[scratch_meq_audio_l];
    audio_out_r = &block.state.own_scratch[scratch_meq_audio_r];
    block.state.own_audio[0][0][0].fill(0.0f);
    block.state.own_audio[0][0][1].fill(0.0f);
  } else
    for (int c = 0; c < 2; c++)
      audio_in[c].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][0][c]);

  int meq_flt_mode0 = block_auto[param_meq_flt_mode][0].step();
  switch (meq_flt_mode0)
  {
  case meq_flt_mode_off: active_filters--;  break;
  case meq_flt_mode_lpf: process_meq_single_filter<0, meq_flt_mode_lpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hpf: process_meq_single_filter<0, meq_flt_mode_hpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bpf: process_meq_single_filter<0, meq_flt_mode_bpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bsf: process_meq_single_filter<0, meq_flt_mode_bsf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_apf: process_meq_single_filter<0, meq_flt_mode_apf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_peq: process_meq_single_filter<0, meq_flt_mode_peq>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bll: process_meq_single_filter<0, meq_flt_mode_bll>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_lsh: process_meq_single_filter<0, meq_flt_mode_lsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hsh: process_meq_single_filter<0, meq_flt_mode_hsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  }
  if(is_parallel && meq_flt_mode0 != meq_flt_mode_off)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][0][f] += (*audio_out_l)[f];
      block.state.own_audio[0][0][1][f] += (*audio_out_r)[f];
    }

  int meq_flt_mode1 = block_auto[param_meq_flt_mode][1].step();
  switch (meq_flt_mode1)
  {
  case meq_flt_mode_off: active_filters--;  break;
  case meq_flt_mode_lpf: process_meq_single_filter<1, meq_flt_mode_lpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hpf: process_meq_single_filter<1, meq_flt_mode_hpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bpf: process_meq_single_filter<1, meq_flt_mode_bpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bsf: process_meq_single_filter<1, meq_flt_mode_bsf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_apf: process_meq_single_filter<1, meq_flt_mode_apf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_peq: process_meq_single_filter<1, meq_flt_mode_peq>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bll: process_meq_single_filter<1, meq_flt_mode_bll>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_lsh: process_meq_single_filter<1, meq_flt_mode_lsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hsh: process_meq_single_filter<1, meq_flt_mode_hsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  }
  if (is_parallel && meq_flt_mode1 != meq_flt_mode_off)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][0][f] += (*audio_out_l)[f];
      block.state.own_audio[0][0][1][f] += (*audio_out_r)[f];
    }

  int meq_flt_mode2 = block_auto[param_meq_flt_mode][2].step();
  switch (meq_flt_mode2)
  {
  case meq_flt_mode_off: active_filters--;  break;
  case meq_flt_mode_lpf: process_meq_single_filter<2, meq_flt_mode_lpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hpf: process_meq_single_filter<2, meq_flt_mode_hpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bpf: process_meq_single_filter<2, meq_flt_mode_bpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bsf: process_meq_single_filter<2, meq_flt_mode_bsf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_apf: process_meq_single_filter<2, meq_flt_mode_apf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_peq: process_meq_single_filter<2, meq_flt_mode_peq>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bll: process_meq_single_filter<2, meq_flt_mode_bll>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_lsh: process_meq_single_filter<2, meq_flt_mode_lsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hsh: process_meq_single_filter<2, meq_flt_mode_hsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  }
  if (is_parallel && meq_flt_mode2 != meq_flt_mode_off)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][0][f] += (*audio_out_l)[f];
      block.state.own_audio[0][0][1][f] += (*audio_out_r)[f];
    }

  int meq_flt_mode3 = block_auto[param_meq_flt_mode][3].step();
  switch (meq_flt_mode3)
  {
  case meq_flt_mode_off: active_filters--;  break;
  case meq_flt_mode_lpf: process_meq_single_filter<3, meq_flt_mode_lpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hpf: process_meq_single_filter<3, meq_flt_mode_hpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bpf: process_meq_single_filter<3, meq_flt_mode_bpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bsf: process_meq_single_filter<3, meq_flt_mode_bsf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_apf: process_meq_single_filter<3, meq_flt_mode_apf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_peq: process_meq_single_filter<3, meq_flt_mode_peq>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bll: process_meq_single_filter<3, meq_flt_mode_bll>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_lsh: process_meq_single_filter<3, meq_flt_mode_lsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hsh: process_meq_single_filter<3, meq_flt_mode_hsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  }
  if (is_parallel && meq_flt_mode3 != meq_flt_mode_off)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][0][f] += (*audio_out_l)[f];
      block.state.own_audio[0][0][1][f] += (*audio_out_r)[f];
    }

  int meq_flt_mode4 = block_auto[param_meq_flt_mode][4].step();
  switch (meq_flt_mode4)
  {
  case meq_flt_mode_off: active_filters--;  break;
  case meq_flt_mode_lpf: process_meq_single_filter<4, meq_flt_mode_lpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hpf: process_meq_single_filter<4, meq_flt_mode_hpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bpf: process_meq_single_filter<4, meq_flt_mode_bpf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bsf: process_meq_single_filter<4, meq_flt_mode_bsf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_apf: process_meq_single_filter<4, meq_flt_mode_apf>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_peq: process_meq_single_filter<4, meq_flt_mode_peq>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_bll: process_meq_single_filter<4, meq_flt_mode_bll>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_lsh: process_meq_single_filter<4, meq_flt_mode_lsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  case meq_flt_mode_hsh: process_meq_single_filter<4, meq_flt_mode_hsh>(block, modulation, *audio_in_l, *audio_in_r, *audio_out_l, *audio_out_r); break;
  }
  if (is_parallel && meq_flt_mode4 != meq_flt_mode_off)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][0][f] += (*audio_out_l)[f];
      block.state.own_audio[0][0][1][f] += (*audio_out_r)[f];
    }

  if(is_parallel && active_filters > 1)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      block.state.own_audio[0][0][0][f] /= active_filters;
      block.state.own_audio[0][0][1][f] /= active_filters;
    }
}

void
fx_engine::process_delay(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  bool sync = block_auto[param_dly_sync][0].step() != 0;
  if(sync) process_delay_sync<true>(block, audio_in, modulation);
  else process_delay_sync<false>(block, audio_in, modulation);
}

template <bool Sync>
void fx_engine::process_delay_sync(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  switch (block.state.own_block_automation[param_dly_mode][0].step())
  {
  case dly_mode_fdbk: process_dly_fdbk_sync<Sync>(block, audio_in, modulation); break;
  case dly_mode_multi: process_dly_multi_sync<Sync>(block, audio_in, modulation); break;
  default: assert(false); break;
  }
}

template <bool Sync>
void fx_engine::process_dly_fdbk_sync(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  float const max_feedback = 0.99f;
  auto const& block_auto = block.state.own_block_automation;

  auto& l_time_curve = block.state.own_scratch[scratch_dly_fdbk_l];
  auto& r_time_curve = block.state.own_scratch[scratch_dly_fdbk_r];
  auto const& amt_curve = *modulation[module_gfx][block.module_slot][param_dly_amt][0];
  auto const& mix_curve = *modulation[module_gfx][block.module_slot][param_dly_mix][0];
  auto const& spread_curve = *modulation[module_gfx][block.module_slot][param_dly_sprd][0];
  
  if constexpr (Sync)
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

    // double mod is needed to not go negative
    float dry_l = audio_in[0][f];
    float dry_r = audio_in[1][f];
    float wet_l0 = _dly_buffer[0][(((_dly_pos - l_time_samples_0) % _dly_capacity) + _dly_capacity) % _dly_capacity];
    float wet_l1 = _dly_buffer[0][(((_dly_pos - l_time_samples_1) % _dly_capacity) + _dly_capacity) % _dly_capacity];
    float wet_l_base = ((1 - l_time_t) * wet_l0 + l_time_t * wet_l1) * amt_curve[f] * max_feedback;
    float wet_r0 = _dly_buffer[1][(((_dly_pos - r_time_samples_0) % _dly_capacity) + _dly_capacity) % _dly_capacity];
    float wet_r1 = _dly_buffer[1][(((_dly_pos - r_time_samples_1) % _dly_capacity) + _dly_capacity) % _dly_capacity];
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

template <bool Sync>
void fx_engine::process_dly_multi_sync(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  int tap_count = block_auto[param_dly_multi_taps][0].step();
  auto& time_curve = block.state.own_scratch[scratch_dly_multi_time];
  auto& hold_curve = block.state.own_scratch[scratch_dly_multi_hold];
  auto const& amt_curve = *modulation[module_gfx][block.module_slot][param_dly_amt][0];
  auto const& mix_curve = *modulation[module_gfx][block.module_slot][param_dly_mix][0];

  auto& spread_curve = block.state.own_scratch[scratch_dly_multi_sprd];
  auto const& spread_curve_norm = *modulation[module_gfx][block.module_slot][param_dly_sprd][0];
  block.normalized_to_raw_block<domain_type::linear>(module_gfx, param_dly_sprd, spread_curve_norm, spread_curve);
  
  if constexpr (Sync)
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
    float spread = spread_curve[f];
    float time_samples_t = time_curve[f] * block.sample_rate;
    float hold_samples_t = hold_curve[f] * block.sample_rate;

    for (int c = 0; c < 2; c++)
    {
      float wet = 0.0f;
      float tap_amt = 1.0f - (1.0f - amt_curve[f]) * (1.0f - amt_curve[f]);
      for (int t = 0; t < tap_count; t++)
      {
        int lr = (t + c) % 2;
        float tap_bal = stereo_balance2(lr, spread);
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
  double freq_hz, double res, float& left, float& right)
{
  double const max_res = 0.99;
  double w = pi64 * freq_hz / (block.sample_rate * oversmp_factor);
  _dst_svf.init_lpf(w, res * max_res);
  left = _dst_svf.next(0, left);
  right = _dst_svf.next(1, right);
}

template <bool Graph> void
fx_engine::process_dist(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  auto const& block_auto = block.state.own_block_automation;
  switch (block_auto[param_dist_mode][0].step())
  {
  case dist_mode_no_filter: process_dist_mode<Graph, dist_mode_no_filter>(block, audio_in, modulation); break;
  case dist_mode_filt_to_shape: process_dist_mode<Graph, dist_mode_filt_to_shape>(block, audio_in, modulation); break;
  case dist_mode_shape_to_filt: process_dist_mode<Graph, dist_mode_shape_to_filt>(block, audio_in, modulation); break;
  default: assert(false); break;
  }
}

// https://dafx.de/paper-archive/2012/papers/dafx12_submission_45.pdf
template <bool Graph, int Mode> void
fx_engine::process_dist_mode(plugin_block& block, 
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation)
{
  switch (block.state.own_block_automation[param_dist_skew_x][0].step())
  {
  case wave_skew_type_off: process_dist_mode_x<Graph, Mode>(
    block, audio_in, modulation, wave_skew_bi_off); break;
  case wave_skew_type_lin: process_dist_mode_x<Graph, Mode>(
    block, audio_in, modulation, wave_skew_bi_lin); break;
  case wave_skew_type_scu: process_dist_mode_x<Graph, Mode>(
    block, audio_in, modulation, wave_skew_bi_scu); break;
  case wave_skew_type_scb: process_dist_mode_x<Graph, Mode>(
    block, audio_in, modulation, wave_skew_bi_scb); break;
  case wave_skew_type_xpu: process_dist_mode_x<Graph, Mode>(
    block, audio_in, modulation, wave_skew_bi_xpu); break;
  case wave_skew_type_xpb: process_dist_mode_x<Graph, Mode>(
    block, audio_in, modulation, wave_skew_bi_xpb); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Mode, class SkewX> void 
fx_engine::process_dist_mode_x(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x)
{
  switch (block.state.own_block_automation[param_dist_skew_y][0].step())
  {
  case wave_skew_type_off: process_dist_mode_xy<Graph, Mode, SkewX>(
    block, audio_in, modulation, skew_x, wave_skew_bi_off); break;
  case wave_skew_type_lin: process_dist_mode_xy<Graph, Mode, SkewX>(
    block, audio_in, modulation, skew_x, wave_skew_bi_lin); break;
  case wave_skew_type_scu: process_dist_mode_xy<Graph, Mode, SkewX>(
    block, audio_in, modulation, skew_x, wave_skew_bi_scu); break;
  case wave_skew_type_scb: process_dist_mode_xy<Graph, Mode, SkewX>(
    block, audio_in, modulation, skew_x, wave_skew_bi_scb); break;
  case wave_skew_type_xpu: process_dist_mode_xy<Graph, Mode, SkewX>(
    block, audio_in, modulation, skew_x, wave_skew_bi_xpu); break;
  case wave_skew_type_xpb: process_dist_mode_xy<Graph, Mode, SkewX>(
    block, audio_in, modulation, skew_x, wave_skew_bi_xpb); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Mode, class SkewX, class SkewY> void 
fx_engine::process_dist_mode_xy(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x, SkewY skew_y)
{
  // from here we deviate for DSF distortion
  // shaper is the DSF function and clip is fixed to tanh
  auto const& block_auto = block.state.own_block_automation;
  if (block.state.own_block_automation[param_type][0].step() == type_dsf_dst)
  {
    float dsf_dist = block_auto[param_dist_dsf_dist][0].real();
    int dsf_parts = (int)std::round(block_auto[param_dist_dsf_parts][0].real());
    process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, false>(
      block, audio_in, modulation, skew_x, skew_y, 
      [](float in, float exp) { return std::tanh(in); },
      [dsf_dist, dsf_parts](float in, float inc, float sr, float dsf_freq, float dsf_dcy) {
      // input may exceed -1/+1, need to get into 0..1 to use as a phase
      // todo select pre-clipper and dsf frequency
      // also less partials
      return generate_dsf(bipolar_to_unipolar(std::tanh(in)), inc, sr, dsf_freq, dsf_parts, dsf_dist, dsf_dcy); });
    return;
  }

  switch (block_auto[param_dist_clip][0].step())
  {
  case dist_clip_tanh: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, false>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    return std::tanh(in);
    }); break;
  case dist_clip_hard: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, false>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    return std::clamp(in, -1.0f, 1.0f);
    }); break;
  case dist_clip_inv: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, false>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    return signum(in) * (1.0f - (1.0f / (1.0f + std::fabs(30.0f * in))));
    }); break;
  case dist_clip_sin: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, false>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    float sgn = signum(in);
    if (std::fabs(in) > 2.0f / 3.0f) return sgn;
    return std::sin((in * 3.0f * pi32) / 4.0f);
    }); break;
  case dist_clip_exp: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, true>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    float sgn = signum(in);
    if (std::fabs(in) > 2.0f / 3.0f) return sgn;
    return sgn * (1.0f - std::pow(std::fabs(1.5f * in - sgn), exp));
    }); break;
  case dist_clip_cube: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, false>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    float sgn = signum(in);
    if (std::fabs(in) > 2.0f / 3.0f) return sgn;
    return (9 * in * 0.25f) - (27 * in * in * in / 16.0f);
    }); break;
  case dist_clip_tsq: process_dist_mode_xy_clip<Graph, Mode, SkewX, SkewY, false>(
    block, audio_in, modulation, skew_x, skew_y, [](float in, float exp) {
    float sgn = signum(in);
    if (std::fabs(in) > 2.0f / 3.0f) return sgn;
    if (-1.0f / 3.0f < in && in < 1.0f / 3.0f) return 2.0f * in;
    float y = 2.0f - std::fabs(3.0f * in);
    return sgn * (3.0f - y * y) / 3.0f;
    }); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Mode, class SkewX, class SkewY, bool ClipIsExp, class Clip> void 
fx_engine::process_dist_mode_xy_clip(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x, SkewY skew_y, Clip clip)
{
  switch (block.state.own_block_automation[param_dist_shaper][0].step())
  {
  case wave_shape_type_saw: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_saw); break;
  case wave_shape_type_tri: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_tri); break;
  case wave_shape_type_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin); break;
  case wave_shape_type_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos); break;
  case wave_shape_type_sin_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin_sin); break;
  case wave_shape_type_sin_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin_cos); break;
  case wave_shape_type_cos_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos_sin); break;
  case wave_shape_type_cos_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos_cos); break;
  case wave_shape_type_sin_sin_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin_sin_sin); break;
  case wave_shape_type_sin_sin_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin_sin_cos); break;
  case wave_shape_type_sin_cos_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin_cos_sin); break;
  case wave_shape_type_sin_cos_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_sin_cos_cos); break;
  case wave_shape_type_cos_sin_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos_sin_sin); break;
  case wave_shape_type_cos_sin_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos_sin_cos); break;
  case wave_shape_type_cos_cos_sin: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos_cos_sin); break;
  case wave_shape_type_cos_cos_cos: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_cos_cos_cos); break;
  case wave_shape_type_sqr_or_fold: process_dist_mode_xy_clip_shape<Graph, Mode, SkewX, SkewY, ClipIsExp, Clip>(
    block, audio_in, modulation, skew_x, skew_y, clip, wave_shape_bi_fold); break;
  default: assert(false); break;
  }
}

template <bool Graph, int Mode, class SkewX, class SkewY, bool ClipIsExp, class Clip, class Shape> void 
fx_engine::process_dist_mode_xy_clip_shape(plugin_block& block,
  jarray<float, 2> const& audio_in, cv_audio_matrix_mixdown const& modulation, SkewX skew_x, SkewY skew_y, Clip clip, Shape shape)
{
  int this_module = _global ? module_gfx : module_vfx;
  auto const& block_auto = block.state.own_block_automation;
  int oversmp_stages = block_auto[param_dist_over][0].step();
  int oversmp_factor = 1 << oversmp_stages;
  int skew_x_type = block_auto[param_dist_skew_x][0].step();
  int skew_y_type = block_auto[param_dist_skew_y][0].step();

  auto const& mix_curve = *modulation[this_module][block.module_slot][param_dist_mix][0];
  auto const& res_curve = *modulation[this_module][block.module_slot][param_dist_lp_res][0];
  auto const& dsf_dcy_curve = *modulation[this_module][block.module_slot][param_dist_dsf_dcy][0];
  auto const& x_curve_plain = *modulation[this_module][block.module_slot][param_dist_skew_x_amt][0];
  auto const& y_curve_plain = *modulation[this_module][block.module_slot][param_dist_skew_y_amt][0];

  jarray<float, 1> const* x_curve = &x_curve_plain;
  if(wave_skew_is_exp(skew_x_type))
  {
    auto& x_scratch = block.state.own_scratch[scratch_dist_x];
    for (int f = block.start_frame; f < block.end_frame; f++)
      x_scratch[f] = std::log(0.001 + (x_curve_plain[f] * 0.98)) / log_half;
    x_curve = &x_scratch;
  }

  jarray<float, 1> const* y_curve = &y_curve_plain;
  if (wave_skew_is_exp(skew_y_type))
  {
    auto& y_scratch = block.state.own_scratch[scratch_dist_y];
    for (int f = block.start_frame; f < block.end_frame; f++)
      y_scratch[f] = std::log(0.001 + (y_curve_plain[f] * 0.98)) / log_half;
    y_curve = &y_scratch;
  }

  auto& gain_curve = block.state.own_scratch[scratch_dist_gain_raw];
  auto const& gain_curve_plain = *modulation[this_module][block.module_slot][param_dist_gain][0];
  block.normalized_to_raw_block<domain_type::log>(this_module, param_dist_gain, gain_curve_plain, gain_curve);

  auto& freq_curve = block.state.own_scratch[scratch_dist_svf_freq];
  auto const& freq_curve_plain = *modulation[this_module][block.module_slot][param_dist_lp_frq][0];
  if constexpr (Mode == dist_mode_filt_to_shape || Mode == dist_mode_shape_to_filt)
    block.normalized_to_raw_block<domain_type::log>(this_module, param_dist_lp_frq, freq_curve_plain, freq_curve);

  auto& clip_exp_curve = block.state.own_scratch[scratch_dist_clip_exp];
  auto const& clip_exp_curve_plain = *modulation[this_module][block.module_slot][param_dist_clip_exp][0];
  if constexpr (ClipIsExp)
    block.normalized_to_raw_block<domain_type::linear>(this_module, param_dist_clip_exp, clip_exp_curve_plain, clip_exp_curve);

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

  // only used for dsf
  //float dsf_freq = 1000.0f; // TODO
  float ovrsmp_rate = block.sample_rate * oversmp_factor;
  //float ovrsmp_inc = dsf_freq / ovrsmp_rate;

  _dst_oversampler.process(oversmp_stages, lanes, 1, block.start_frame, block.end_frame, true, [&](float** lanes_channels, int frame)
    { 
      float left_in = lanes_channels[0][frame];
      float right_in = lanes_channels[1][frame];
      float& left = lanes_channels[0][frame];
      float& right = lanes_channels[1][frame];

      // oversampler is from 0 to (end_frame - start_frame) * oversmp_factor
      // all the not-oversampled stuff requires from start_frame to end_frame
      // so mind the bookkeeping
      int mod_index = block.start_frame + frame / oversmp_factor;

      float dsf_freq = gain_curve[mod_index] * 2000;
      float ovrsmp_inc = dsf_freq / ovrsmp_rate; // TODO
      // ALSO TODO make dist/osc dsf dist fake modulatable param

      left = skew_x(left * gain_curve[mod_index], (*x_curve)[mod_index]);
      right = skew_x(right * gain_curve[mod_index], (*x_curve)[mod_index]);
      if constexpr(Mode == dist_mode_filt_to_shape)
        dist_svf_next(block, oversmp_factor, freq_curve[mod_index], res_curve[mod_index], left, right);
      left = shape(left, ovrsmp_inc, ovrsmp_rate, dsf_freq, dsf_dcy_curve[mod_index]);
      right = shape(right, ovrsmp_inc, ovrsmp_rate, dsf_freq, dsf_dcy_curve[mod_index]);
      if constexpr (Mode == dist_mode_shape_to_filt)
        dist_svf_next(block, oversmp_factor, freq_curve[mod_index], res_curve[mod_index], left, right);

      float exp = 0.0f;
      if constexpr (ClipIsExp)
        exp = clip_exp_curve[mod_index];

      left = clip(skew_y(left, (*y_curve)[mod_index]), exp);
      right = clip(skew_y(right, (*y_curve)[mod_index]), exp);

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