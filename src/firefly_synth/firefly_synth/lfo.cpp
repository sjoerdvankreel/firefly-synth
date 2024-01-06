#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/waves.hpp>
#include <firefly_synth/synth.hpp>
#include <firefly_synth/smooth_noise.hpp>

#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

static float const max_filter_time_ms = 500; 
static float const log_half = std::log(0.5f);

enum class lfo_group { phased, noise };
enum class lfo_stage { cycle, filter, end };

enum { section_mode, section_type };
enum { scratch_time, scratch_count };
enum { mode_off, mode_rate, mode_rate_one, mode_rate_wrap, mode_sync, mode_sync_one, mode_sync_wrap };
enum { param_mode, param_rate, param_tempo, param_type, param_filter, param_phase, param_seed, param_steps, param_x, param_y };

#if 0
enum { 
  type_trig_sin, type_trig_cos, 
  type_trig_sin_sin, type_trig_sin_cos, type_trig_cos_sin, type_trig_cos_cos,
  type_trig_sin_sin_sin, type_trig_sin_sin_cos, type_trig_sin_cos_sin, type_trig_sin_cos_cos,
  type_trig_cos_sin_sin, type_trig_cos_sin_cos, type_trig_cos_cos_sin, type_trig_cos_cos_cos,

  type_trig_log_sin, type_trig_log_cos,
  type_trig_log_sin_sin, type_trig_log_sin_cos, type_trig_log_cos_sin, type_trig_log_cos_cos,
  type_trig_log_sin_sin_sin, type_trig_log_sin_sin_cos, type_trig_log_sin_cos_sin, type_trig_log_sin_cos_cos,
  type_trig_log_cos_sin_sin, type_trig_log_cos_sin_cos, type_trig_log_cos_cos_sin, type_trig_log_cos_cos_cos,
  
  type_other_skew, type_other_pulse, type_other_pulse_lin, type_other_tri, type_other_tri_log, type_other_saw, type_other_saw_lin, type_other_saw_log,
  type_static, type_static_add, type_static_free, type_static_add_free, 
  type_smooth, type_smooth_log };
#endif

static bool is_noise(int type) { return false; }

static bool is_one_shot_full(int mode) { return mode == mode_rate_one || mode == mode_sync_one; }
static bool is_one_shot_wrapped(int mode) { return mode == mode_rate_wrap || mode == mode_sync_wrap; }
static bool is_sync(int mode) { return mode == mode_sync || mode == mode_sync_one || mode == mode_sync_wrap; }

#if 0
static bool is_static_add(int type) { return type == type_static_add || type == type_static_add_free; }
static bool is_static_free(int type) { return type == type_static_free || type == type_static_add_free; }
static bool is_noise(int type) { 
  return type == type_static || type == type_static_add || type == type_static_free || 
  type == type_static_add_free || type == type_smooth || type == type_smooth_log; }

static bool is_trig_log(int type) { return type_trig_log_sin <= type && type <= type_trig_log_cos_cos_cos; }
static bool has_x(int type) { 
  return is_trig_log(type) || type == type_other_skew || 
  type == type_other_tri_log || type == type_other_saw_lin || type == type_other_pulse_lin; }
static bool has_y(int type) { 
  return is_trig_log(type) || type == type_other_skew || type == type_other_tri_log || type == type_other_saw_lin || 
  type == type_other_saw_log || type == type_other_pulse_lin || type == type_static_add || type == type_static_add_free || type == type_smooth_log; }
#endif

#if 0
static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{3B223DEC-1085-4D44-9C16-05B7FAA22006}", "Sin");
  result.emplace_back("{1EAC4007-3122-4064-9B97-AFF28C902400}", "Cos");
  result.emplace_back("{7A510580-5E85-4B49-A4D2-54269B50FF62}", "SinSin");
  result.emplace_back("{335FD8C1-3162-4A75-8EFA-6C8240EAEA78}", "SinCos");
  result.emplace_back("{C01104A8-A299-4D8E-AC94-D81387D66B2A}", "CosSin");
  result.emplace_back("{FEEEEADE-119C-461D-A6F7-5A29BF0088F6}", "CosCos");
  result.emplace_back("{11B30ACD-5B04-406A-8B3B-E9B7A9C62AF5}", "SSS");
  result.emplace_back("{8BF5A3AE-D56C-490B-B6F6-22BDFF1913EB}", "SSC");
  result.emplace_back("{74E9F12C-91BD-4027-BCA4-842BF6B23367}", "SCS");
  result.emplace_back("{EE7C1611-D741-46EF-A398-2B5F62749471}", "SCC");
  result.emplace_back("{4BA55E18-2937-4F21-8BC5-CDDD9DEAEF93}", "CSS");
  result.emplace_back("{A86309CC-EC83-41A6-8BBB-81A15DF42676}", "CSC");
  result.emplace_back("{B6A7D580-F6BC-4C4B-B749-E2FCBD10F6F3}", "CCS");
  result.emplace_back("{B3780540-3B43-43D8-8859-14F69C846055}", "CCC");

  result.emplace_back("{95A5E2C4-06E5-445B-A612-ED1AC091546B}", "Sin.Log");
  result.emplace_back("{F867923B-EAAF-45B5-B54F-E60D06601952}", "Cos.Log");
  result.emplace_back("{E0495ACF-3393-4BD5-BB30-91D68B02813E}", "SS.Log");
  result.emplace_back("{97F4AE1E-AA72-4749-8F8C-FE6F074068BE}", "SC.Log");
  result.emplace_back("{83923F08-D9E3-4E86-AF4A-1C7A57B079B3}", "CS.Log");
  result.emplace_back("{B37645EF-AE72-4D46-9ADE-E8E1A075BCA6}", "CC.Log");

  result.emplace_back("{EE47DB48-5FA4-4FBD-A39C-A5263C9B4A44}", "SSS.Log");
  result.emplace_back("{1262E2B1-51F2-4FDF-8ECA-0632DF1AD168}", "SSC.Log");
  result.emplace_back("{0AD50684-D536-458D-935E-FB946DD55CEC}", "SCS.Log");
  result.emplace_back("{226BFA21-906F-4948-B0F3-AED0C474DE5A}", "SCC.Log");
  result.emplace_back("{EC830EE3-67CE-43EE-BA5D-E08FAF950C04}", "CSS.Log");
  result.emplace_back("{B7F41A67-511E-4DF8-84F5-E2A3495610A0}", "CSC.Log");
  result.emplace_back("{49D64771-94C6-4BA1-BACF-80C75F078FE6}", "CCS.Log");
  result.emplace_back("{749AB04D-6902-4C69-85DF-9D7B5A422C4F}", "CCC.Log");
  
  result.emplace_back("{3AF5A419-583F-435E-ABF7-BA514FA608C9}", "Skew");
  result.emplace_back("{34480144-5349-49C1-9211-4CED6E6C8203}", "Pulse");
  result.emplace_back("{91CB7634-9759-485A-9DFF-6F5F86966212}", "Pulse.Lin");
  result.emplace_back("{42E1F070-191B-411A-9FFD-D966990B9712}", "Tri");
  result.emplace_back("{B6775D3D-D12B-4326-9414-18788BBC4898}", "Tri.Log");
  result.emplace_back("{2190619A-CB71-47F3-9B93-364BF4DA6BE6}", "Saw");
  result.emplace_back("{C304D3F4-3D77-437D-BFBD-4BBDA2FC90A5}", "Saw.Lin");
  result.emplace_back("{C91E269F-E83D-41A6-8C64-C34DBF9144C1}", "Saw.Log");

  result.emplace_back("{48D681E8-16C2-42FB-B64F-146C7F689C45}", "Static");
  result.emplace_back("{6182E4EF-93F2-4CDC-A015-7437C05F7E70}", "Stat.Add");
  result.emplace_back("{5AA8914D-CF39-4435-B374-B4C66002DC8B}", "Stat.Free");
  result.emplace_back("{4B9E5DE0-9F32-4EB0-949F-1108B2A21DC1}", "St.AddFr");

  result.emplace_back("{4F079460-C774-4B69-BCCA-3065BE26D28F}", "Smt");
  result.emplace_back("{92856FAE-84EE-42B9-926D-7F4FA7AE21E9}", "Smt.Log");
  return result;
}
#endif

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{E8D04800-17A9-42AB-9CAE-19322A400334}", "Off");
  result.emplace_back("{5F57863F-4157-4F53-BB02-C6693675B881}", "Rate");
  result.emplace_back("{0A5F479F-9180-4498-9464-DBEA0595C86B}", "Rate.One");
  result.emplace_back("{12E9AF37-1C1F-43AB-9405-86F103293C4C}", "Rate.Wrap");
  result.emplace_back("{E2692483-F48B-4037-BF74-64BB62110538}", "Sync");
  result.emplace_back("{85B1AC0B-FA06-4E23-A7EF-3EBF6F620948}", "Sync.One");
  result.emplace_back("{9CFBC6ED-1024-4FDE-9291-9280FDA9BC1E}", "Sync.Wrap");
  return result;
}

class lfo_engine :
public module_engine {
  float _phase;
  float _ref_phase;
  float _static_level;
  float _lfo_end_value;
  float _log_skew_x_exp;
  float _log_skew_y_exp;
  float _filter_end_value;
  
  bool const _global;
  lfo_stage _stage = {};
  cv_filter _filter = {};
  smooth_noise _smooth_noise;
  std::uint32_t _static_state;

  int _static_dir = 1;
  int _static_step_pos = 0;
  int _noise_total_pos = 0;
  int _static_step_samples = 0;
  int _noise_total_samples = 0;
  int _end_filter_pos = 0;
  int _end_filter_stage_samples = 0;

  std::vector<multi_menu_item> _type_items;

  void reset_noise(int seed, int steps);
  void update_block_params(plugin_block const* block);

  float calc_smooth(int seed, int steps);
  float calc_static(bool one_shot, bool add, bool free, float y, int seed, int steps);

  template <lfo_group Group, class Calc>
  void process_loop(plugin_block& block, Calc calc);

  void process_phased(plugin_block& block);
  template <class Shape>
  void process_phased_shape(plugin_block& block, Shape shape);
  template <class Shape, class SkewX>
  void process_phased_shape_x(plugin_block& block, Shape shape, SkewX skew_x);
  template <class Shape, class SkewX, class SkewY>
  void process_phased_shape_xy(plugin_block& block, Shape shape, SkewX skew_x, SkewY skew_y);

public:
  PB_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  void reset(plugin_block const*) override;
  lfo_engine(bool global, std::vector<multi_menu_item> const& type_items) : 
  _global(global), _smooth_noise(1, 1), _type_items(type_items) {}

  void process(plugin_block& block) override;
};

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_glfo, 0, param_mode, 0, "Sync");
  state.set_text_at(module_glfo, 0, param_tempo, 0, "3/1");
  state.set_text_at(module_glfo, 1, param_mode, 0, "Rate");
}

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result = {};
  result.bpm = 120;
  result.max_frame_count = 200;
  return result;
}

static std::unique_ptr<graph_engine>
make_graph_engine(plugin_desc const* desc)
{
  auto params = make_graph_engine_params();
  return std::make_unique<graph_engine>(desc, params);
}

float
lfo_frequency_from_state(plugin_state const& state, int module_index, int module_slot, int bpm)
{
  int mode = state.get_plain_at(module_index, module_slot, param_mode, 0).step();
  return sync_or_freq_from_state(state, 120, is_sync(mode), module_index, module_slot, param_rate, param_tempo);
}

static graph_data
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int mode = state.get_plain_at(mapping.module_index, mapping.module_slot, param_mode, mapping.param_slot).step();
  if(mode == mode_off)
    return graph_data(graph_data_type::off, {});
  
  int sample_rate = -1;
  std::string partition;
  auto const params = make_graph_engine_params();
  float freq = lfo_frequency_from_state(state, mapping.module_index, mapping.module_slot, 120);

  if (!is_sync(mode))
  {
    // 1 second, or the minimum that shows 1 cycle
    if (freq >= 1)
    {
      partition = "1 Sec";
      sample_rate = params.max_frame_count;
    }
    else
    {
      sample_rate = params.max_frame_count * freq;
      partition = float_to_string(1 / freq, 2) + " Sec";
    }
  }
  
  // draw synced 1/1 as full cycle
  if (is_sync(mode))
  {
    // 1 bar, or the minimum that shows 1 cycle
    float one_bar_freq = timesig_to_freq(120, { 1, 1 });
    if(freq >= one_bar_freq)
    {
      partition = "1 Bar";
      sample_rate = one_bar_freq * params.max_frame_count;
    }
    else
    {
      sample_rate = params.max_frame_count * freq;
      partition = float_to_string(one_bar_freq / freq, 2) + " Bar";
    }
  }

  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  auto const* block = engine->process_default(mapping.module_index, mapping.module_slot);
  engine->process_end();
  jarray<float, 1> series(block->state.own_cv[0][0]);
  return graph_data(series, false, { partition });
}

module_topo
lfo_topo(int section, gui_colors const& colors, gui_position const& pos, bool global)
{
  auto type_menu = make_wave_multi_menu();
  auto const voice_info = make_topo_info("{58205EAB-FB60-4E46-B2AB-7D27F069CDD3}", "Voice LFO", "V.LFO", true, true, module_vlfo, 6);
  auto const global_info = make_topo_info("{FAF92753-C6E4-4D78-BD7C-584EF473E29F}", "Global LFO", "G.LFO", true, true, module_glfo, 6);
  module_stage stage = global ? module_stage::input : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 1, {
      make_module_dsp_output(true, make_topo_info("{197CB1D4-8A48-4093-A5E7-2781C731BBFC}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { 5, 12 } })));
  
  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_graph_engine;
  if(global) result.default_initializer = init_global_default;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [global, type_items = type_menu.multi_items](auto const&, int, int) {
    return std::make_unique<lfo_engine>(global, type_items); };

  result.sections.emplace_back(make_param_section(section_mode,
    make_topo_tag("{F0002F24-0CA7-4DF3-A5E3-5B33055FD6DC}", "Mode"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1 }))));
  auto& mode = result.params.emplace_back(make_param(
    make_topo_info("{252D76F2-8B36-4F15-94D0-2E974EC64522}", "Mode", param_mode, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(mode_items(), ""),
    make_param_gui_single(section_mode, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  mode.gui.submenu = std::make_shared<gui_submenu>();
  mode.gui.submenu->indices.push_back(mode_off);
  mode.gui.submenu->add_submenu("Rate", { mode_rate, mode_rate_one, mode_rate_wrap });
  mode.gui.submenu->add_submenu("Sync", { mode_sync, mode_sync_one, mode_sync_wrap });
  auto& rate = result.params.emplace_back(make_param(
    make_topo_info("{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_log(0.01, 20, 1, 1, 2, "Hz"),
    make_param_gui_single(section_mode, gui_edit_type::hslider, { 0, 1 }, gui_label_contents::value, make_label_none())));
  rate.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  rate.gui.bindings.visible.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_sync && vs[0] != mode_sync_one && vs[0] != mode_sync_wrap; });
  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Tempo", param_tempo, 1),
    make_param_dsp_input(!global, param_automate::automate), make_domain_timesig_default(false, { 1, 4 }),
    make_param_gui_single(section_mode, gui_edit_type::list, { 0, 1 }, gui_label_contents::name, make_label_none())));
  tempo.gui.submenu = make_timesig_submenu(tempo.domain.timesigs);
  tempo.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  tempo.gui.bindings.visible.bind_params({ param_mode }, [](auto const& vs) { return vs[0] == mode_sync || vs[0] == mode_sync_one || vs[0] == mode_sync_wrap; });

  result.sections.emplace_back(make_param_section(section_type,
    make_topo_tag("{A5B5DC53-2E73-4C0B-9DD1-721A335EA076}", "Type"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 5, 5, 5, 5, 4, 4 }))));

  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Type", param_type, 1),
    make_param_dsp_input(!global, param_automate::automate), make_domain_item(type_menu.items, "Sin.OfX/OfY"),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  type.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  type.gui.submenu = type_menu.submenu;

#if 0
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->add_submenu("Trig", { type_trig_sin, type_trig_cos,
    type_trig_sin_sin, type_trig_sin_cos, type_trig_cos_sin, type_trig_cos_cos,
    type_trig_sin_sin_sin, type_trig_sin_sin_cos, type_trig_sin_cos_sin, type_trig_sin_cos_cos,
    type_trig_cos_sin_sin, type_trig_cos_sin_cos, type_trig_cos_cos_sin, type_trig_cos_cos_cos });
  type.gui.submenu->add_submenu("Trig.Log", { type_trig_log_sin, type_trig_log_cos,
  type_trig_log_sin_sin, type_trig_log_sin_cos, type_trig_log_cos_sin, type_trig_log_cos_cos,
  type_trig_log_sin_sin_sin, type_trig_log_sin_sin_cos, type_trig_log_sin_cos_sin, type_trig_log_sin_cos_cos,
  type_trig_log_cos_sin_sin, type_trig_log_cos_sin_cos, type_trig_log_cos_cos_sin, type_trig_log_cos_cos_cos });
  type.gui.submenu->add_submenu("Smooth", { type_smooth, type_smooth_log });
  type.gui.submenu->add_submenu("Static", { type_static, type_static_add, type_static_free, type_static_add_free });
  type.gui.submenu->add_submenu("Other", { type_other_skew, type_other_pulse, type_other_pulse_lin, type_other_tri, 
    type_other_tri_log, type_other_saw, type_other_saw_lin, type_other_saw_log });
#endif

  auto& smooth = result.params.emplace_back(make_param(
    make_topo_info("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", "Sm", true, true, param_filter, 1),
    make_param_dsp_input(!global, param_automate::automate), make_domain_linear(0, max_filter_time_ms, 0, 0, "Ms"),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  smooth.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& phase = result.params.emplace_back(make_param(
    make_topo_info("{B23E9732-ECE3-4D5D-8EC1-FF299C6926BB}", "Phase", "Ph", true, true, param_phase, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  phase.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info("{19ED9A71-F50A-47D6-BF97-70EA389A62EA}", "Seed", "Sd", true, true, param_seed, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  seed.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& steps = result.params.emplace_back(make_param(
    make_topo_info("{445CF696-0364-4638-9BD5-3E1C9A957B6A}", "Steps", "St", true, true, param_steps, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_step(2, 99, 4, 0),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  steps.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& x = result.params.emplace_back(make_param(
    make_topo_info("{8CEDE705-8901-4247-9854-83FB7BEB14F9}", "X", "X", true, true, param_x, 1),
    make_param_dsp_input(!global, param_automate::automate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 5 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  //x.gui.bindings.enabled.bind_params({ param_mode, param_type }, [](auto const& vs) { return vs[0] != mode_off && has_x(vs[1]); });
  (void)x;
  auto& y = result.params.emplace_back(make_param(
    make_topo_info("{8939B05F-8677-4AA9-8C4C-E6D96D9AB640}", "Y", "Y", true, true, param_y, 1),
    make_param_dsp_input(!global, param_automate::automate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 6 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  //y.gui.bindings.enabled.bind_params({ param_mode, param_type }, [](auto const& vs) { return vs[0] != mode_off && has_y(vs[1]); });
  (void)y;

  return result;
}

#if false
static float
skew_log(float in, float exp)
{ return std::pow(in, exp); }

static float
calc_trig_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_01(phase)); }
static float
calc_trig_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_01(phase)); }
static float
calc_trig_sin_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_sin_01(phase)); }
static float
calc_trig_sin_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_cos_01(phase)); }
static float
calc_trig_cos_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_sin_01(phase)); }
static float
calc_trig_cos_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_cos_01(phase)); }
static float
calc_trig_sin_sin_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_sin_sin_01(phase)); }
static float
calc_trig_sin_sin_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_sin_cos_01(phase)); }
static float
calc_trig_sin_cos_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_cos_sin_01(phase)); }
static float
calc_trig_sin_cos_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(sin_cos_cos_01(phase)); }
static float
calc_trig_cos_sin_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_sin_sin_01(phase)); }
static float
calc_trig_cos_sin_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_sin_cos_01(phase)); }
static float
calc_trig_cos_cos_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_cos_sin_01(phase)); }
static float
calc_trig_cos_cos_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return bipolar_to_unipolar(cos_cos_cos_01(phase)); }

static float
calc_trig_log_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_sin_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_sin_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_cos_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_cos_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_sin_sin_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_sin_sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_sin_sin_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_sin_cos_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_sin_cos_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_cos_sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_sin_cos_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(sin_cos_cos_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos_sin_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_sin_sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos_sin_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_sin_cos_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos_cos_sin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_cos_sin_01(skew_log(phase, x_exp))), y_exp); }
static float
calc_trig_log_cos_cos_cos(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(bipolar_to_unipolar(cos_cos_cos_01(skew_log(phase, x_exp))), y_exp); }

static float
calc_other_skew(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(std::fabs(phase - skew_log(phase, x_exp)), y_exp); }
static float
calc_other_saw(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return phase; }
static float
calc_other_saw_lin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return phase < x? phase * y / x : y +  (phase - x) / ( 1 - x) * (1 - y); }
static float
calc_other_saw_log(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(calc_other_saw(phase, x, y, x_exp, y_exp, seed, steps), y_exp); }
static float
calc_other_pulse(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return phase < 0.5? 0: 1; }
static float
calc_other_pulse_lin(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return phase < x? 0: y; }
static float
calc_other_tri(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return 1 - std::fabs(unipolar_to_bipolar(calc_other_saw(phase, x, y, x_exp, y_exp, seed, steps))); }
static float
calc_other_tri_log(float phase, float x, float y, float x_exp, float y_exp, int seed, int steps)
{ return skew_log(calc_other_tri(skew_log(phase, x_exp), x, y, x_exp, y_exp, seed, steps), y_exp); }

#endif

float
lfo_engine::calc_smooth(int seed, int steps)
{
  float result = _smooth_noise.next((float)_noise_total_pos / (_noise_total_samples + 1) * steps);
  if(_noise_total_pos++ >= _noise_total_samples)
    reset_noise(seed, steps);
  return result;
}

float 
lfo_engine::calc_static(bool one_shot, bool add, bool free, float y, int seed, int steps)
{
  float result = _static_level;
  _static_step_pos++;
  _noise_total_pos++;
  if (!one_shot && _noise_total_pos >= _noise_total_samples)
  {
    _noise_total_pos = 0;
    if(!free) reset_noise(seed, steps);
  }
  else if (_static_step_pos >= _static_step_samples)
  {
    if(add)
    {
      _static_level = _static_level + fast_rand_next(_static_state) * unipolar_to_bipolar(y) * _static_dir;
      if(_static_level < 0 || _static_level > 1)
      {
        _static_dir *= -1;
        _static_level -= 2 * (_static_level - (int)_static_level);
      }
    } else
      _static_level = fast_rand_next(_static_state);
    _static_step_pos = 0;
  }
  return result;
}

void
lfo_engine::reset_noise(int seed, int steps)
{
  _static_dir = 1;
  _static_step_pos = 0;
  _noise_total_pos = 0;
  _static_state = fast_rand_seed(seed);
  _static_level = fast_rand_next(_static_state);
  _smooth_noise = smooth_noise(seed, steps);
}

void 
lfo_engine::update_block_params(plugin_block const* block)
{
  auto const& block_auto = block->state.own_block_automation;
  float x = block_auto[param_x][0].real();
  float y = block_auto[param_y][0].real();
  float filter = block_auto[param_filter][0].real();
  _log_skew_x_exp = std::log(0.001 + (x * 0.999)) / log_half;
  _log_skew_y_exp = std::log(0.001 + (y * 0.999)) / log_half;
  _filter.set(block->sample_rate, filter / 1000.0f);
}

void
lfo_engine::reset(plugin_block const* block) 
{ 
  _ref_phase = 0;
  _lfo_end_value = 0;
  _end_filter_pos = 0;
  _filter_end_value = 0;
  _stage = lfo_stage::cycle;
  _end_filter_stage_samples = 0;
  
  update_block_params(block);
  auto const& block_auto = block->state.own_block_automation;
  _phase = _global? 0: block_auto[param_phase][0].real();
  if (is_noise(block_auto[param_type][0].step())) _phase = 0;  
  reset_noise(block_auto[param_seed][0].step(), block_auto[param_steps][0].step());
}

void
lfo_engine::process(plugin_block& block)
{
  auto const& block_auto = block.state.own_block_automation;
  int mode = block_auto[param_mode][0].step();
  if (mode == mode_off)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  if(_stage == lfo_stage::end)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _filter_end_value);
    return; 
  }

  if(_global) update_block_params(&block);
  process_phased(block);

#if 0
  if (sx == wave_skew_type_off && sy == wave_skew_type_off && shp == wave_shape_type_saw) process_phased(block, wave_skew_off, wave_skew_off, wave_shape_saw);
  if (sx == wave_skew_type_off && sy == wave_skew_type_off && shp == wave_shape_type_sqr) process_phased(block, wave_skew_off, wave_skew_off, wave_shape_sqr);
  if (sx == wave_skew_type_off && sy == wave_skew_type_off && shp == wave_shape_type_sin) process_phased(block, wave_skew_off, wave_skew_off, wave_shape_sin);
  if (sx == wave_skew_type_off && sy == wave_skew_type_scu && shp == wave_shape_type_saw) process_phased(block, wave_skew_off, wave_skew_scu, wave_shape_saw);
  if (sx == wave_skew_type_off && sy == wave_skew_type_scu && shp == wave_shape_type_sqr) process_phased(block, wave_skew_off, wave_skew_scu, wave_shape_sqr);
  if (sx == wave_skew_type_off && sy == wave_skew_type_scu && shp == wave_shape_type_sin) process_phased(block, wave_skew_off, wave_skew_scu, wave_shape_sin);
  if (sx == wave_skew_type_off && sy == wave_skew_type_scb && shp == wave_shape_type_saw) process_phased(block, wave_skew_off, wave_skew_scb, wave_shape_saw);
  if (sx == wave_skew_type_off && sy == wave_skew_type_scb && shp == wave_shape_type_sqr) process_phased(block, wave_skew_off, wave_skew_scb, wave_shape_sqr);
  if (sx == wave_skew_type_off && sy == wave_skew_type_scb && shp == wave_shape_type_sin) process_phased(block, wave_skew_off, wave_skew_scb, wave_shape_sin);
  if (sx == wave_skew_type_off && sy == wave_skew_type_lin && shp == wave_shape_type_saw) process_phased(block, wave_skew_off, wave_skew_lin, wave_shape_saw);
  if (sx == wave_skew_type_off && sy == wave_skew_type_lin && shp == wave_shape_type_sqr) process_phased(block, wave_skew_off, wave_skew_lin, wave_shape_sqr);
  if (sx == wave_skew_type_off && sy == wave_skew_type_lin && shp == wave_shape_type_sin) process_phased(block, wave_skew_off, wave_skew_lin, wave_shape_sin);
  if (sx == wave_skew_type_off && sy == wave_skew_type_exp && shp == wave_shape_type_saw) process_phased(block, wave_skew_off, wave_skew_exp, wave_shape_saw);
  if (sx == wave_skew_type_off && sy == wave_skew_type_exp && shp == wave_shape_type_sqr) process_phased(block, wave_skew_off, wave_skew_exp, wave_shape_sqr);
  if (sx == wave_skew_type_off && sy == wave_skew_type_exp && shp == wave_shape_type_sin) process_phased(block, wave_skew_off, wave_skew_exp, wave_shape_sin);

  if (sx == wave_skew_type_scu && sy == wave_skew_type_off && shp == wave_shape_type_saw) process_phased(block, wave_skew_scu, wave_skew_off, wave_shape_saw);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_off && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scu, wave_skew_off, wave_shape_sqr);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_off && shp == wave_shape_type_sin) process_phased(block, wave_skew_scu, wave_skew_off, wave_shape_sin);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_scu && shp == wave_shape_type_saw) process_phased(block, wave_skew_scu, wave_skew_scu, wave_shape_saw);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_scu && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scu, wave_skew_scu, wave_shape_sqr);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_scu && shp == wave_shape_type_sin) process_phased(block, wave_skew_scu, wave_skew_scu, wave_shape_sin);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_scb && shp == wave_shape_type_saw) process_phased(block, wave_skew_scu, wave_skew_scb, wave_shape_saw);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_scb && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scu, wave_skew_scb, wave_shape_sqr);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_scb && shp == wave_shape_type_sin) process_phased(block, wave_skew_scu, wave_skew_scb, wave_shape_sin);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_lin && shp == wave_shape_type_saw) process_phased(block, wave_skew_scu, wave_skew_lin, wave_shape_saw);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_lin && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scu, wave_skew_lin, wave_shape_sqr);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_lin && shp == wave_shape_type_sin) process_phased(block, wave_skew_scu, wave_skew_lin, wave_shape_sin);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_exp && shp == wave_shape_type_saw) process_phased(block, wave_skew_scu, wave_skew_exp, wave_shape_saw);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_exp && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scu, wave_skew_exp, wave_shape_sqr);
  if (sx == wave_skew_type_scu && sy == wave_skew_type_exp && shp == wave_shape_type_sin) process_phased(block, wave_skew_scu, wave_skew_exp, wave_shape_sin);

  if (sx == wave_skew_type_scb && sy == wave_skew_type_off && shp == wave_shape_type_saw) process_phased(block, wave_skew_scb, wave_skew_off, wave_shape_saw);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_off && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scb, wave_skew_off, wave_shape_sqr);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_off && shp == wave_shape_type_sin) process_phased(block, wave_skew_scb, wave_skew_off, wave_shape_sin);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_scu && shp == wave_shape_type_saw) process_phased(block, wave_skew_scb, wave_skew_scu, wave_shape_saw);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_scu && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scb, wave_skew_scu, wave_shape_sqr);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_scu && shp == wave_shape_type_sin) process_phased(block, wave_skew_scb, wave_skew_scu, wave_shape_sin);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_scb && shp == wave_shape_type_saw) process_phased(block, wave_skew_scb, wave_skew_scb, wave_shape_saw);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_scb && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scb, wave_skew_scb, wave_shape_sqr);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_scb && shp == wave_shape_type_sin) process_phased(block, wave_skew_scb, wave_skew_scb, wave_shape_sin);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_lin && shp == wave_shape_type_saw) process_phased(block, wave_skew_scb, wave_skew_lin, wave_shape_saw);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_lin && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scb, wave_skew_lin, wave_shape_sqr);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_lin && shp == wave_shape_type_sin) process_phased(block, wave_skew_scb, wave_skew_lin, wave_shape_sin);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_exp && shp == wave_shape_type_saw) process_phased(block, wave_skew_scb, wave_skew_exp, wave_shape_saw);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_exp && shp == wave_shape_type_sqr) process_phased(block, wave_skew_scb, wave_skew_exp, wave_shape_sqr);
  if (sx == wave_skew_type_scb && sy == wave_skew_type_exp && shp == wave_shape_type_sin) process_phased(block, wave_skew_scb, wave_skew_exp, wave_shape_sin);

  if (sx == wave_skew_type_lin && sy == wave_skew_type_off && shp == wave_shape_type_saw) process_phased(block, wave_skew_lin, wave_skew_off, wave_shape_saw);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_off && shp == wave_shape_type_sqr) process_phased(block, wave_skew_lin, wave_skew_off, wave_shape_sqr);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_off && shp == wave_shape_type_sin) process_phased(block, wave_skew_lin, wave_skew_off, wave_shape_sin);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_scu && shp == wave_shape_type_saw) process_phased(block, wave_skew_lin, wave_skew_scu, wave_shape_saw);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_scu && shp == wave_shape_type_sqr) process_phased(block, wave_skew_lin, wave_skew_scu, wave_shape_sqr);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_scu && shp == wave_shape_type_sin) process_phased(block, wave_skew_lin, wave_skew_scu, wave_shape_sin);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_scb && shp == wave_shape_type_saw) process_phased(block, wave_skew_lin, wave_skew_scb, wave_shape_saw);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_scb && shp == wave_shape_type_sqr) process_phased(block, wave_skew_lin, wave_skew_scb, wave_shape_sqr);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_scb && shp == wave_shape_type_sin) process_phased(block, wave_skew_lin, wave_skew_scb, wave_shape_sin);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_lin && shp == wave_shape_type_saw) process_phased(block, wave_skew_lin, wave_skew_lin, wave_shape_saw);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_lin && shp == wave_shape_type_sqr) process_phased(block, wave_skew_lin, wave_skew_lin, wave_shape_sqr);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_lin && shp == wave_shape_type_sin) process_phased(block, wave_skew_lin, wave_skew_lin, wave_shape_sin);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_exp && shp == wave_shape_type_saw) process_phased(block, wave_skew_lin, wave_skew_exp, wave_shape_saw);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_exp && shp == wave_shape_type_sqr) process_phased(block, wave_skew_lin, wave_skew_exp, wave_shape_sqr);
  if (sx == wave_skew_type_lin && sy == wave_skew_type_exp && shp == wave_shape_type_sin) process_phased(block, wave_skew_lin, wave_skew_exp, wave_shape_sin);

  if (sx == wave_skew_type_exp && sy == wave_skew_type_off && shp == wave_shape_type_saw) process_phased(block, wave_skew_exp, wave_skew_off, wave_shape_saw);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_off && shp == wave_shape_type_sqr) process_phased(block, wave_skew_exp, wave_skew_off, wave_shape_sqr);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_off && shp == wave_shape_type_sin) process_phased(block, wave_skew_exp, wave_skew_off, wave_shape_sin);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_scu && shp == wave_shape_type_saw) process_phased(block, wave_skew_exp, wave_skew_scu, wave_shape_saw);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_scu && shp == wave_shape_type_sqr) process_phased(block, wave_skew_exp, wave_skew_scu, wave_shape_sqr);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_scu && shp == wave_shape_type_sin) process_phased(block, wave_skew_exp, wave_skew_scu, wave_shape_sin);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_scb && shp == wave_shape_type_saw) process_phased(block, wave_skew_exp, wave_skew_scb, wave_shape_saw);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_scb && shp == wave_shape_type_sqr) process_phased(block, wave_skew_exp, wave_skew_scb, wave_shape_sqr);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_scb && shp == wave_shape_type_sin) process_phased(block, wave_skew_exp, wave_skew_scb, wave_shape_sin);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_lin && shp == wave_shape_type_saw) process_phased(block, wave_skew_exp, wave_skew_lin, wave_shape_saw);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_lin && shp == wave_shape_type_sqr) process_phased(block, wave_skew_exp, wave_skew_lin, wave_shape_sqr);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_lin && shp == wave_shape_type_sin) process_phased(block, wave_skew_exp, wave_skew_lin, wave_shape_sin);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_exp && shp == wave_shape_type_saw) process_phased(block, wave_skew_exp, wave_skew_exp, wave_shape_saw);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_exp && shp == wave_shape_type_sqr) process_phased(block, wave_skew_exp, wave_skew_exp, wave_shape_sqr);
  if (sx == wave_skew_type_exp && sy == wave_skew_type_exp && shp == wave_shape_type_sin) process_phased(block, wave_skew_exp, wave_skew_exp, wave_shape_sin);

  switch (type)
  {

  case type_trig_sin: process_loop<lfo_group::phased>(block, calc_trig_sin); break;
  case type_trig_cos: process_loop<lfo_group::phased>(block, calc_trig_cos); break;
  case type_trig_sin_sin: process_loop<lfo_group::phased>(block, calc_trig_sin_sin); break;
  case type_trig_sin_cos: process_loop<lfo_group::phased>(block, calc_trig_sin_cos); break;
  case type_trig_cos_sin: process_loop<lfo_group::phased>(block, calc_trig_cos_sin); break;
  case type_trig_cos_cos: process_loop<lfo_group::phased>(block, calc_trig_cos_cos); break;
  case type_trig_sin_sin_sin: process_loop<lfo_group::phased>(block, calc_trig_sin_sin_sin); break;
  case type_trig_sin_sin_cos: process_loop<lfo_group::phased>(block, calc_trig_sin_sin_cos); break;
  case type_trig_sin_cos_sin: process_loop<lfo_group::phased>(block, calc_trig_sin_cos_sin); break;
  case type_trig_sin_cos_cos: process_loop<lfo_group::phased>(block, calc_trig_sin_cos_cos); break;
  case type_trig_cos_sin_sin: process_loop<lfo_group::phased>(block, calc_trig_cos_sin_sin); break;
  case type_trig_cos_sin_cos: process_loop<lfo_group::phased>(block, calc_trig_cos_sin_cos); break;
  case type_trig_cos_cos_sin: process_loop<lfo_group::phased>(block, calc_trig_cos_cos_sin); break;
  case type_trig_cos_cos_cos: process_loop<lfo_group::phased>(block, calc_trig_cos_cos_cos); break;

  case type_trig_log_sin: process_loop<lfo_group::phased>(block, calc_trig_log_sin); break;
  case type_trig_log_cos: process_loop<lfo_group::phased>(block, calc_trig_log_cos); break;
  case type_trig_log_sin_sin: process_loop<lfo_group::phased>(block, calc_trig_log_sin_sin); break;
  case type_trig_log_sin_cos: process_loop<lfo_group::phased>(block, calc_trig_log_sin_cos); break;
  case type_trig_log_cos_sin: process_loop<lfo_group::phased>(block, calc_trig_log_cos_sin); break;
  case type_trig_log_cos_cos: process_loop<lfo_group::phased>(block, calc_trig_log_cos_cos); break;
  case type_trig_log_sin_sin_sin: process_loop<lfo_group::phased>(block, calc_trig_log_sin_sin_sin); break;
  case type_trig_log_sin_sin_cos: process_loop<lfo_group::phased>(block, calc_trig_log_sin_sin_cos); break;
  case type_trig_log_sin_cos_sin: process_loop<lfo_group::phased>(block, calc_trig_log_sin_cos_sin); break;
  case type_trig_log_sin_cos_cos: process_loop<lfo_group::phased>(block, calc_trig_log_sin_cos_cos); break;
  case type_trig_log_cos_sin_sin: process_loop<lfo_group::phased>(block, calc_trig_log_cos_sin_sin); break;
  case type_trig_log_cos_sin_cos: process_loop<lfo_group::phased>(block, calc_trig_log_cos_sin_cos); break;
  case type_trig_log_cos_cos_sin: process_loop<lfo_group::phased>(block, calc_trig_log_cos_cos_sin); break;
  case type_trig_log_cos_cos_cos: process_loop<lfo_group::phased>(block, calc_trig_log_cos_cos_cos); break;

  case type_other_skew: process_loop<lfo_group::phased>(block, calc_other_skew); break;
  case type_other_tri: process_loop<lfo_group::phased>(block, calc_other_tri); break;
  case type_other_tri_log: process_loop<lfo_group::phased>(block, calc_other_tri_log); break;
  case type_other_pulse: process_loop<lfo_group::phased>(block, calc_other_pulse); break;
  case type_other_pulse_lin: process_loop<lfo_group::phased>(block, calc_other_pulse_lin); break;
  case type_other_saw: process_loop<lfo_group::phased>(block, calc_other_saw); break;
  case type_other_saw_lin: process_loop<lfo_group::phased>(block, calc_other_saw_lin); break;
  case type_other_saw_log: process_loop<lfo_group::phased>(block, calc_other_saw_log); break;

  case type_static: case type_static_add: case type_static_free: case type_static_add_free:
    process_loop<lfo_group::noise>(block, [this, mode, type](float phase, float x, float y, float x_exp, float y_exp, int seed, int steps) {
      return calc_static(is_one_shot_full(mode), is_static_add(type), is_static_free(type), y, seed, steps); }); break;
  case type_smooth:
    process_loop<lfo_group::noise>(block, [this](float phase, float x, float y, float x_exp, float y_exp, int seed, int steps) {
      return calc_smooth(seed, steps); }); break;
  case type_smooth_log:
    process_loop<lfo_group::noise>(block, [this](float phase, float x, float y, float x_exp, float y_exp, int seed, int steps) {
      return skew_log(calc_smooth(seed, steps), y_exp); }); break;
  case 0: break;
  default: break;
  }
#endif
}

void 
lfo_engine::process_phased(plugin_block& block)
{
  switch (_type_items[block.state.own_block_automation[param_type][0].step()].index1)
  {
  case wave_shape_type_saw: process_phased_shape(block, wave_shape_saw); break;
  case wave_shape_type_sqr: process_phased_shape(block, wave_shape_sqr); break;
  case wave_shape_type_sin: process_phased_shape(block, wave_shape_sin); break;
  default: assert(false); break;
  }
}

template <class Shape> void 
lfo_engine::process_phased_shape(plugin_block& block, Shape shape)
{
  switch (_type_items[block.state.own_block_automation[param_type][0].step()].index2)
  {
  case wave_skew_type_off: process_phased_shape_x(block, shape, wave_skew_off); break;
  case wave_skew_type_lin: process_phased_shape_x(block, shape, wave_skew_lin); break;
  case wave_skew_type_scu: process_phased_shape_x(block, shape, wave_skew_scu); break;
  case wave_skew_type_scb: process_phased_shape_x(block, shape, wave_skew_scb); break;
  case wave_skew_type_xpu: process_phased_shape_x(block, shape, wave_skew_xpu); break;
  case wave_skew_type_xpb: process_phased_shape_x(block, shape, wave_skew_xpb); break;
  default: assert(false); break;
  }
}

template <class Shape, class SkewX> void 
lfo_engine::process_phased_shape_x(plugin_block& block, Shape shape, SkewX skew_x)
{
  switch (_type_items[block.state.own_block_automation[param_type][0].step()].index3)
  {
  case wave_skew_type_off: process_phased_shape_xy(block, shape, skew_x, wave_skew_off); break;
  case wave_skew_type_lin: process_phased_shape_xy(block, shape, skew_x, wave_skew_lin); break;
  case wave_skew_type_scu: process_phased_shape_xy(block, shape, skew_x, wave_skew_scu); break;
  case wave_skew_type_scb: process_phased_shape_xy(block, shape, skew_x, wave_skew_scb); break;
  case wave_skew_type_xpu: process_phased_shape_xy(block, shape, skew_x, wave_skew_xpu); break;
  case wave_skew_type_xpb: process_phased_shape_xy(block, shape, skew_x, wave_skew_xpb); break;
  default: assert(false); break;
  }
}

template <class Shape, class SkewX, class SkewY> void
lfo_engine::process_phased_shape_xy(plugin_block& block, Shape shape, SkewX skew_x, SkewY skew_y)
{
  auto const& block_auto = block.state.own_block_automation;
  int type = block_auto[param_type][0].step();
  auto const& type_item = _type_items[type];
  int sx = type_item.index1;
  int sy = type_item.index2;
  float x = block_auto[param_x][0].real();
  float y = block_auto[param_y][0].real();
  float px = wave_skew_is_exp(sx)? _log_skew_x_exp: x;
  float py = wave_skew_is_exp(sy) ? _log_skew_y_exp : y;
  auto processor = [px, py, skew_x, skew_y, shape](float in) { return wave_calc_unipolar(in, px, py, shape, skew_x, skew_y); };
  process_loop<lfo_group::phased>(block, processor);
}

template <lfo_group Group, class Calc>
void lfo_engine::process_loop(plugin_block& block, Calc calc)
{
  int this_module = _global ? module_glfo : module_vlfo;
  auto const& block_automation = block.state.own_block_automation;
  //float x = block_automation[param_x][0].real();
  //float y = block_automation[param_y][0].real();
  int mode = block_automation[param_mode][0].step();
  //int seed = block_automation[param_seed][0].step();
  int steps = block_automation[param_steps][0].step();
  auto const& rate_curve = sync_or_freq_into_scratch(block, is_sync(mode), this_module, param_rate, param_tempo, scratch_time);

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    if (_stage == lfo_stage::end)
    {
      block.state.own_cv[0][0][f] = _filter_end_value;
      continue;
    }

    if (_stage == lfo_stage::filter)
    {
      _filter_end_value = _filter.next(_lfo_end_value);
      block.state.own_cv[0][0][f] = _filter_end_value;
      if (_end_filter_pos++ >= _end_filter_stage_samples)
        _stage = lfo_stage::end;
      continue;
    }

    if constexpr(Group == lfo_group::noise)
    {
      _noise_total_samples = std::ceil(block.sample_rate / rate_curve[f]);
      _static_step_samples = std::ceil(block.sample_rate / (rate_curve[f] * steps));
    }
    
    //_lfo_end_value = calc(_phase, x, y, _log_skew_x_exp, _log_skew_y_exp, seed, steps);
    _lfo_end_value = calc(_phase);
    _filter_end_value = _filter.next(check_unipolar(_lfo_end_value));
    block.state.own_cv[0][0][f] = _filter_end_value;
    
    bool phase_wrapped = false;
    if constexpr (Group == lfo_group::phased)
      phase_wrapped = increment_and_wrap_phase(_phase, rate_curve[f], block.sample_rate);
    bool ref_wrapped = increment_and_wrap_phase(_ref_phase, rate_curve[f], block.sample_rate);

    bool ended = ref_wrapped && is_one_shot_full(mode);
    if constexpr (Group == lfo_group::phased)
      ended |= phase_wrapped && is_one_shot_wrapped(mode);
    if (ended)
    {
      _stage = lfo_stage::filter;
      float filter_ms = block_automation[param_filter][0].real();
      _end_filter_stage_samples = block.sample_rate * filter_ms * 0.001;
    }
  }
}

}
