#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth {

static float const comb_max_ms = 5;

enum { section_type, section_svf, section_comb, section_delay };
enum { type_off, type_svf_lpf, type_svf_hpf, type_svf_bpf, type_svf_bsf, type_svf_apf, 
  type_svf_peq, type_svf_bll, type_svf_lsh, type_svf_hsh, type_comb, type_delay };
enum { param_type, param_svf_freq, param_svf_res, param_svf_kbd, param_svf_gain, param_comb_dly_plus, 
  param_comb_gain_plus, param_comb_dly_min, param_comb_gain_min, param_delay_tempo, param_delay_feedback };

static bool is_svf(int type) { return type_svf_lpf <= type && type <= type_svf_hsh; }
static bool is_svf_gain(int type) { return type_svf_bll <= type && type <= type_svf_hsh; }

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
  result.emplace_back("{8140F8BC-E4FD-48A1-B147-CD63E9616450}", "Comb");
  if(global) result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  return result;
}

class fx_engine: 
public module_engine {  

  bool const _global;

  // svf
  double _ic1eq[2];
  double _ic2eq[2];

  // delay
  int _pos;
  int const _capacity;
  jarray<float, 2> _buffer = {};

  void process_delay(plugin_block& block, cv_matrix_mixdown const& modulation);
  template <class Init>
  void process_svf(plugin_block& block, cv_matrix_mixdown const& modulation, Init init);

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
  if(type == type_off || type == type_comb) return graph_data(graph_data_type::off, {});

  int frame_count = -1;
  int sample_rate = -1;
  jarray<float, 2> audio_in;
  auto const params = make_graph_engine_params();
  if(is_svf(type))
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
    engine.reset(global? nullptr: &block);
    cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
    engine.process(block, &modulation, &audio_in);
  });
  engine->process_end();

  if (type == type_delay)
  {
    std::vector<float> series(block->state.own_audio[0][0][0].begin(), block->state.own_audio[0][0][0].begin() + frame_count);
    return graph_data(jarray<float, 1>(series), true, { "IR" });
  }

  // remap over 0.8 just to look pretty
  std::vector<float> response(log_remap_series_x(fft(block->state.own_audio[0][0][0].data()), 0.8f));
  return graph_data(jarray<float, 1>(response), false, { "FR" });
}

module_topo
fx_topo(int section, gui_colors const& colors, gui_position const& pos, bool global)
{
  auto const voice_info = make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Voice FX", "V.FX", true, module_vfx, 10);
  auto const global_info = make_topo_info("{31EF3492-FE63-4A59-91DA-C2B4DD4A8891}", "Global FX", "G.FX", true, module_gfx, 10);
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
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  type.gui.submenu->indices.push_back(type_comb);
  type.gui.submenu->add_submenu("SVF", { type_svf_lpf, type_svf_hpf, type_svf_bpf, type_svf_bsf, type_svf_apf, type_svf_peq, type_svf_bll, type_svf_lsh, type_svf_hsh });
  if(global) type.gui.submenu->indices.push_back(type_delay);

  auto& svf = result.sections.emplace_back(make_param_section(section_svf,
    make_topo_tag("{DFA6BD01-8F89-42CB-9D0E-E1902193DD5E}", "SVF"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 4, 1, 1, 1 } })));
  svf.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf(vs[0]); });
  svf.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_off || is_svf(vs[0]); });
  result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", param_svf_freq, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(20, 20000, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_svf, gui_edit_type::hslider, { 0, 0 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "Res", param_svf_res, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  result.params.emplace_back(make_param(
    make_topo_info("{9EEA6FE0-983E-4EC7-A47F-0DFD79D68BCB}", "Kbd", param_svf_kbd, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  auto& svf_gain = result.params.emplace_back(make_param(
    make_topo_info("{FE108A32-770A-415B-9C85-449ABF6A944C}", "Gain", param_svf_gain, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-8, 8, 0, 0, true),
    make_param_gui_single(section_svf, gui_edit_type::knob, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  svf_gain.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return is_svf_gain(vs[0]); });

  auto& comb = result.sections.emplace_back(make_param_section(section_comb,
    make_topo_tag("{54CF060F-3EE7-4F42-921F-612F8EEA8EB0}", "Comb"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 1, 1, 1, 1 } })));
  comb.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_comb; });
  result.params.emplace_back(make_param(
    make_topo_info("{097ECBDB-1129-423C-9335-661D612A9945}", "Delay+", param_comb_dly_plus, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(0, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::knob, { 0, 0 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  result.params.emplace_back(make_param(
    make_topo_info("{3069FB5E-7B17-4FC4-B45F-A9DFA383CAA9}", "Gain+", param_comb_gain_plus, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0.5, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  result.params.emplace_back(make_param(
    make_topo_info("{D4846933-6AED-4979-AA1C-2DD80B68404F}", "Delay-", param_comb_dly_min, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(0, comb_max_ms, 1, 2, "Ms"),
    make_param_gui_single(section_comb, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  result.params.emplace_back(make_param(
    make_topo_info("{9684165E-897B-4EB7-835D-D5AAF8E61E65}", "Gain-", param_comb_gain_min, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_comb, gui_edit_type::knob, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  if(!global) return result;

  auto& delay = result.sections.emplace_back(make_param_section(section_delay,
    make_topo_tag("{E92225CF-21BF-459C-8C9D-8E50285F26D4}", "Delay"),
    make_param_section_gui({ 0, 1 }, { { 1 }, { 1, 1 } })));
  delay.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });
  auto& delay_tempo = result.params.emplace_back(make_param(
    make_topo_info("{C2E282BA-9E4F-4AE6-A055-8B5456780C66}", "Tempo", param_delay_tempo, 1),
    make_param_dsp_input(!global, param_automate::automate), make_domain_timesig_default(false, {3, 16}),
    make_param_gui_single(section_delay, gui_edit_type::list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  delay_tempo.gui.submenu = make_timesig_submenu(delay_tempo.domain.timesigs);
  result.params.emplace_back(make_param(
    make_topo_info("{037E4A64-8F80-4E0A-88A0-EE1BB83C99C6}", "Fdbk", param_delay_feedback, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_delay, gui_edit_type::hslider, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  return result;
}

static void
init_svf_non_shelving(
  double w, double res, double& g, double& k, 
  double& a1, double& a2, double& a3)
{
  g = std::tan(w);
  k = 2 - 2 * res;
  a1 = 1 / (1 + g * (g + k));
  a2 = g * a1;
  a3 = g * a2;
}

static void
init_svf_lpf(
  double w, double res, double& g, double& k,
  double& a1, double& a2, double& a3, 
  double& m0, double& m1, double& m2)
{
  init_svf_non_shelving(w, res, g, k, a1, a2, a3);
  m0 = 0; m1 = 0; m2 = 1;
}

static void
init_svf_hpf(
  double w, double res, double& g, double& k,
  double& a1, double& a2, double& a3, 
  double& m0, double& m1, double& m2)
{
  init_svf_non_shelving(w, res, g, k, a1, a2, a3);
  m0 = 1; m1 = -k; m2 = -1;
}

fx_engine::
fx_engine(bool global, int sample_rate) :
_global(global), _capacity(sample_rate * 10)
{ if(global) _buffer.resize(jarray<int, 1>(2, _capacity)); }

void
fx_engine::reset(plugin_block const*)
{
  _pos = 0;
  _ic1eq[0] = 0;
  _ic1eq[1] = 0;
  _ic2eq[0] = 0;
  _ic2eq[1] = 0;
  if(_global)
    _buffer.fill(0);
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
  if(modulation == nullptr)
    modulation = &get_cv_matrix_mixdown(block, _global);

  switch (type)
  {
  case type_delay: process_delay(block, *modulation); break;
  case type_svf_lpf: process_svf(block, *modulation, init_svf_lpf); break;
  case type_svf_hpf: process_svf(block, *modulation, init_svf_hpf); break;
  }
}

void
fx_engine::process_delay(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  float max_feedback = 0.9f;
  int this_module = _global ? module_gfx : module_vfx;
  float time = get_timesig_time_value(block, this_module, param_delay_tempo);
  int samples = std::min(block.sample_rate * time, (float)_capacity);
  
  auto const& feedback_curve = *modulation[this_module][block.module_slot][param_delay_feedback][0];
  for (int c = 0; c < 2; c++)
    for (int f = block.start_frame; f < block.end_frame; f++)
    {
      float dry = block.state.own_audio[0][0][c][f];
      float wet = _buffer[c][(_pos + f + _capacity - samples) % _capacity];
      block.state.own_audio[0][0][c][f] = dry + wet * feedback_curve[f] * max_feedback;
      _buffer[c][(_pos + f) % _capacity] = block.state.own_audio[0][0][c][f];
    }
  _pos += block.end_frame - block.start_frame;
  _pos %= _capacity;
}
  
template <class Init> void
fx_engine::process_svf(plugin_block& block, cv_matrix_mixdown const& modulation, Init init)
{
  double a1, a2, a3;
  double m0, m1, m2;
  double w, g, k, hz;

  int this_module = _global ? module_gfx : module_vfx;
  auto const& res_curve = *modulation[this_module][block.module_slot][param_svf_res][0];
  auto const& freq_curve = *modulation[this_module][block.module_slot][param_svf_freq][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    hz = block.normalized_to_raw(this_module, param_svf_freq, freq_curve[f]);
    w = pi64 * hz / block.sample_rate;
    init(w, res_curve[f], g, k, a1, a2, a3, m0, m1, m2);
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
