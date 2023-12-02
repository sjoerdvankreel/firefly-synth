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

enum { section_main };
enum { type_off, type_lpf, type_hpf, type_delay };
enum { param_type, param_filter_freq, param_filter_res, param_delay_tempo, param_delay_feedback };

static std::vector<list_item>
type_items(bool global)
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  result.emplace_back("{8B7E5C75-C3F6-4F53-900C-0A75703F5570}", "LPF");
  result.emplace_back("{673C872A-F740-431C-8CD3-F577CE984C2D}", "HPF");
  if(global) result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  return result;
}

class fx_engine: 
public module_engine {  

  bool const _global;

  // filter
  double _ic1eq[2];
  double _ic2eq[2];

  // delay
  int _pos;
  int const _capacity;
  jarray<float, 2> _buffer = {};

  void process_delay(plugin_block& block, cv_matrix_mixdown const& modulation);
  void process_filter(plugin_block& block, cv_matrix_mixdown const& modulation);

public:
  PB_PREVENT_ACCIDENTAL_COPY(fx_engine);
  fx_engine(bool global, int sample_rate);
  
  void initialize() override;
  void process(plugin_block& block, cv_matrix_mixdown const* modulation, jarray<float, 2> const* audio_in);
  void process(plugin_block& block) override { process(block, nullptr, nullptr); }
};

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vfx, 0, param_type, 0, "LPF");
  state.set_text_at(module_vfx, 0, param_filter_res, 0, "50");
  state.set_text_at(module_vfx, 0, param_filter_freq, 0, "20");
}

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_gfx, 0, param_type, 0, "LPF");
  state.set_text_at(module_gfx, 1, param_type, 0, "Delay");
  state.set_text_at(module_gfx, 1, param_delay_tempo, 0, "3/16");
}

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  jarray<float, 2> audio_in;
  graph_engine_params params = {};

  int type = state.get_plain_at(mapping.module_index, mapping.module_slot, param_type, 0).step();
  if(type == type_off) return {};

  params.bpm = 120;
  params.midi_key = midi_middle_c;

  if (type == type_delay)
  {
    int count = 100;
    params.sample_rate = 500;
    params.frame_count = 2000;
    audio_in.resize(jarray<int, 1>(2, params.frame_count));
    for (int i = 0; i < count; i++)
    {
      float sample = std::sin(i / (float)count * pi32 * 2.0f) * (1 - i / (float)count);
      audio_in[0][i] = sample;
      audio_in[1][i] = sample;
    }
  }
  else 
  {
    params.sample_rate = 48000;
    params.frame_count = 48000 / 10;
    audio_in.resize(jarray<int, 1>(2, params.frame_count));
    audio_in[0][0] = 1;
    audio_in[1][0] = 1;
  }

  graph_engine graph_engine(&state, params);
  auto const* block = graph_engine.process(
    mapping.module_index, mapping.module_slot, [mapping, params, &audio_in](plugin_block& block) {
    fx_engine engine(mapping.module_index == module_gfx, params.sample_rate);
    engine.initialize();
    cv_matrix_mixdown modulation(make_static_cv_matrix_mixdown(block));
    engine.process(block, &modulation, &audio_in);
  });

  if (type == type_delay)
    return graph_data(jarray<float, 1>(block->state.own_audio[0][0][0]), true);

  // remap over 0.8 just to look pretty
  std::vector<float> response(log_remap_series_x(fft(block->state.own_audio[0][0][0].data()), 0.8f));
  response.push_back(0);
  response.insert(response.begin(), 0);
  return graph_data(jarray<float, 1>(response), false);
}

module_topo
fx_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos, bool global)
{
  auto const voice_info = make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "VFX", module_vfx, 3);
  auto const global_info = make_topo_info("{31EF3492-FE63-4A59-91DA-C2B4DD4A8891}", "GFX", module_gfx, 3);
  module_stage stage = global ? module_stage::output : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::audio, 0, {
      make_module_dsp_output(false, make_topo_info("{E7C21225-7ED5-45CC-9417-84A69BECA73C}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));
  result.gui.menu_handler_factory = []() { return std::make_unique<routing_tab_menu_handler>(); };

  result.graph_renderer = render_graph;
  if(global) result.default_initializer = init_global_default;
  if(!global) result.default_initializer = init_voice_default;
  result.engine_factory = [global](auto const&, int sample_rate, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<fx_engine>(global, sample_rate); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1 } })));

  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "Type", param_type, 1),
    make_param_dsp_block(param_automate::none), make_domain_item(type_items(global), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  auto filter_menu = std::make_shared<gui_submenu>();
  filter_menu->name = "Filter";
  filter_menu->indices.push_back(type_lpf);
  filter_menu->indices.push_back(type_hpf);
  type.gui.submenu->children.push_back(filter_menu);
  if(global) type.gui.submenu->indices.push_back(type_delay);

  auto& filter_freq = result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", param_filter_freq, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(20, 20000, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  filter_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  filter_freq.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_delay; });

  auto& filter_res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "Res", param_filter_res, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  filter_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  filter_res.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_delay; });

  if(!global) return result;

  auto& delay_tempo = result.params.emplace_back(make_param(
    make_topo_info("{C2E282BA-9E4F-4AE6-A055-8B5456780C66}", "Tempo", param_delay_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 1 }, make_label_none())));
  delay_tempo.gui.submenu = make_timesig_submenu(delay_tempo.domain.timesigs);
  delay_tempo.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });

  auto& delay_gain = result.params.emplace_back(make_param(
    make_topo_info("{037E4A64-8F80-4E0A-88A0-EE1BB83C99C6}", "Fdbk", param_delay_feedback, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  delay_gain.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });

  return result;
}

fx_engine::
fx_engine(bool global, int sample_rate) :
_global(global), _capacity(sample_rate * 10)
{
  if(global) _buffer.resize(jarray<int, 1>(2, _capacity));
  initialize();
}

void
fx_engine::initialize()
{
  _pos = 0;
  _ic1eq[0] = 0;
  _ic1eq[1] = 0;
  _ic2eq[0] = 0;
  _ic2eq[1] = 0;
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
  if (type == type_delay)
    process_delay(block, *modulation);
  if (type == type_lpf || type == type_hpf)
    process_filter(block, *modulation);
}

void
fx_engine::process_delay(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  float max_feedback = 0.9f;
  int this_module = _global ? module_gfx : module_vfx;
  float time = get_timesig_time_value(block, this_module, param_delay_tempo);
  int samples = block.sample_rate * time;
  
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
  
void
fx_engine::process_filter(plugin_block& block, cv_matrix_mixdown const& modulation)
{
  int this_module = _global ? module_gfx : module_vfx;
  int type = block.state.own_block_automation[param_type][0].step();
  auto const& res_curve = *modulation[this_module][block.module_slot][param_filter_res][0];
  auto const& freq_curve = *modulation[this_module][block.module_slot][param_filter_freq][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float freq = block.normalized_to_raw(this_module, param_filter_freq, freq_curve[f]);
    double k = 2 - 2 * res_curve[f];
    double g = std::tan(pi32 * freq / block.sample_rate);
    double a1 = 1 / (1 + g * (g + k));
    double a2 = g * a1;
    double a3 = g * a2;

    for (int c = 0; c < 2; c++)
    {
      double v0 = block.state.own_audio[0][0][c][f];
      double v3 = v0 - _ic2eq[c];
      double v1 = a1 * _ic1eq[c] + a2 * v3;
      double v2 = _ic2eq[c] + a2 * _ic1eq[c] + a3 * v3;
      _ic1eq[c] = 2 * v1 - _ic1eq[c];
      _ic2eq[c] = 2 * v2 - _ic2eq[c];
      float out = type == type_lpf? v2: v0 - k * v1 - v2;
      block.state.own_audio[0][0][c][f] = out;
    }
  }
}

}
