#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

static float const skew_min = 0.0001;
static float const skew_max = 0.9999;
static float const skew_range = skew_max - skew_min;
static float const log_half = std::log(0.5f);

enum { section_mode, section_type };
enum { scratch_time, scratch_count };
enum { mode_off, mode_rate, mode_rate_one, mode_rate_wrap, mode_sync, mode_sync_one, mode_sync_wrap };
enum { param_mode, param_rate, param_tempo, param_type, param_x, param_y, param_filter, param_phase, param_seed };
//enum { type_sine, type_saw, type_sqr, type_skew, type_tri1, type_tri2, type_rnd_y, type_rnd_xy, type_rnd_y_free, type_rnd_xy_free };
enum { 
  type_sin_plain, type_saw_plain, type_sqr_plain, type_tri_plain, 
  type_sin_lin, type_saw_lin, type_sqr_lin, type_tri_lin, 
  type_sin_log, type_saw_log, type_sqr_log, type_tri_log };

static inline bool is_rate(int mode) { return mode == mode_rate || mode == mode_rate_one || mode == mode_rate_wrap; }
static inline bool is_sync(int mode) { return mode == mode_sync || mode == mode_sync_one || mode == mode_sync_wrap; }
static inline bool is_sin(int type) { return type == type_sin_plain || type == type_sin_lin || type == type_sin_log; }
static inline bool is_saw(int type) { return type == type_saw_plain || type == type_saw_lin || type == type_saw_log; }
static inline bool is_sqr(int type) { return type == type_sqr_plain || type == type_sqr_lin || type == type_sqr_log; }
static inline bool is_tri(int type) { return type == type_tri_plain || type == type_tri_lin || type == type_tri_log; }
static inline bool is_log(int type) { return type == type_sin_log || type == type_saw_log || type == type_sqr_log || type == type_tri_log; }
static inline bool is_linear(int type) { return type == type_sin_lin || type == type_saw_lin || type == type_sqr_lin || type == type_tri_lin; }
static inline bool is_plain(int type) { return type == type_sin_plain || type == type_saw_plain || type == type_sqr_plain || type == type_tri_plain; }
static inline bool is_random(int type) { return false; }

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{DE8FF99D-C83F-4723-B8DA-FB1C4877B1F4}", "Sin.Pln");
  result.emplace_back("{01636F45-4734-4762-B475-E4CA15BAE156}", "Saw.Pln");
  result.emplace_back("{497E9796-48D2-4C33-B502-0C3AE3FD03D1}", "Sqr.Pln");
  result.emplace_back("{E1F35D2F-CCFF-46F9-A669-BFC4719211AC}", "Tri.Pln");
  result.emplace_back("{65588A69-520F-4453-850B-A82F6881B7E0}", "Sin.Lin");
  result.emplace_back("{B587443D-2211-4BB7-8105-AA86F5B3820A}", "Saw.Lin");
  result.emplace_back("{37E80713-720D-43AA-A582-B164D6B328D4}", "Sqr.Lin");
  result.emplace_back("{6EEB99FA-0880-49F9-ABE6-5D84C7D8C82B}", "Tri.Lin");
  result.emplace_back("{68DD813A-A699-4AA8-9FF6-0490D2E37858}", "Sin.Log");
  result.emplace_back("{E4E8E1F3-0859-49D6-870D-0609C426BA1B}", "Saw.Log");
  result.emplace_back("{CF0DF88E-0184-436F-9C5B-A1E9AA0DDBD8}", "Sqr.Log");
  result.emplace_back("{08F0C843-2728-4D92-BB2B-F6BC72CEB020}", "Tri.Log");
  //result.emplace_back("{83EF2C08-E5A1-4517-AC8C-D45890936A96}", "Rnd.Y");
  //result.emplace_back("{84BFAC67-D748-4499-813F-0B7FCEBF174B}", "Rnd.XY");
  //result.emplace_back("{F6B72990-D053-4D3A-9B8D-391DDB748DC1}", "RndF.Y");
  //result.emplace_back("{7EBDA9FE-11D9-4C09-BA4B-EF3763FB3CF0}", "RndF.XY");
  return result;
}

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
  bool _ended;
  float _phase;
  float _ref_phase;
  float _end_value;
  bool const _global;
public:
  PB_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  void reset(plugin_block const*) override;
  void process(plugin_block& block) override;
  lfo_engine(bool global) : _global(global) {}
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

static graph_data
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  int mode = state.get_plain_at(mapping.module_index, mapping.module_slot, param_mode, mapping.param_slot).step();
  if(mode == mode_off)
    return graph_data(graph_data_type::off, {});
  
  std::string partition = "1 Sec";
  auto const params = make_graph_engine_params();
  int sample_rate = params.max_frame_count;
  
  // draw synced 1/4 as full cycle
  if (is_sync(mode))
  {
    partition = "1 Bar";
    float one_bar_freq = timesig_to_freq(120, { 1, 1 });
    sample_rate = one_bar_freq * params.max_frame_count;
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
  auto const voice_info = make_topo_info("{58205EAB-FB60-4E46-B2AB-7D27F069CDD3}", "Voice LFO", "V.LFO", true, module_vlfo, 6);
  auto const global_info = make_topo_info("{FAF92753-C6E4-4D78-BD7C-584EF473E29F}", "Global LFO", "G.LFO", true, module_glfo, 6);
  module_stage stage = global ? module_stage::input : module_stage::voice;
  auto const info = topo_info(global ? global_info : voice_info);

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 1, {
      make_module_dsp_output(true, make_topo_info("{197CB1D4-8A48-4093-A5E7-2781C731BBFC}", "Output", 0, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { 3, 5 } })));
  
  result.graph_renderer = render_graph;
  result.graph_engine_factory = make_graph_engine;
  if(global) result.default_initializer = init_global_default;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [global](auto const&, int, int) { return std::make_unique<lfo_engine>(global); };

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
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(0.1, 20, 1, 2, "Hz"),
    make_param_gui_single(section_mode, gui_edit_type::knob, { 0, 1 }, gui_label_contents::none,
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  rate.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  rate.gui.bindings.visible.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_sync && vs[0] != mode_sync_one && vs[0] != mode_sync_wrap; });
  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Tempo", param_tempo, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_timesig_default(false, { 1, 4 }),
    make_param_gui_single(section_mode, gui_edit_type::list, { 0, 1 }, gui_label_contents::name, make_label_none())));
  tempo.gui.submenu = make_timesig_submenu(tempo.domain.timesigs);
  tempo.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  tempo.gui.bindings.visible.bind_params({ param_mode }, [](auto const& vs) { return vs[0] == mode_sync || vs[0] == mode_sync_one || vs[0] == mode_sync_wrap; });
  
  result.sections.emplace_back(make_param_section(section_type,
    make_topo_tag("{A5B5DC53-2E73-4C0B-9DD1-721A335EA076}", "Type"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 3, 3, 5, 5 }))));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Type", param_type, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(type_items(), ""),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  type.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->add_submenu("Plain", {type_sin_plain, type_saw_plain, type_sqr_plain, type_tri_plain });
  type.gui.submenu->add_submenu("Linear", { type_sin_lin, type_saw_lin, type_sqr_lin, type_tri_lin });
  type.gui.submenu->add_submenu("Log", { type_sin_log, type_saw_log, type_sqr_log, type_tri_log });
  // TODO type.gui.submenu->add_submenu("Random", { type_rnd_y, type_rnd_xy, type_rnd_y_free, type_rnd_xy_free });

  auto& x = result.params.emplace_back(make_param(
    make_topo_info("{8CEDE705-8901-4247-9854-83FB7BEB14F9}", "X", "X", true, param_x, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  x.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& y = result.params.emplace_back(make_param(
    make_topo_info("{8939B05F-8677-4AA9-8C4C-E6D96D9AB640}", "Y", "Y", true, param_y, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  y.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& smooth = result.params.emplace_back(make_param(
    make_topo_info("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", "Smt", true, param_filter, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  smooth.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& phase = result.params.emplace_back(make_param(
    make_topo_info("{B23E9732-ECE3-4D5D-8EC1-FF299C6926BB}", "Phase", param_phase, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  phase.gui.submenu = make_midi_note_submenu();
  phase.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  phase.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return !is_random(vs[0]); });
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info("{9F5BE73B-20C0-44C5-B078-CD571497A837}", "Seed", param_seed, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  seed.gui.submenu = make_midi_note_submenu();
  seed.gui.label_reference_text = phase.info.tag.name;
  seed.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  seed.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return is_random(vs[0]); });

  return result;
}

static inline float
skew_x_none(float phase, float x)
{ return phase; }

static inline float
skew_x_log(float phase, float x)
{
  float x_bounded = skew_min + x * skew_range;
  return std::pow(phase, std::log(x_bounded) / log_half);
}

static inline float
skew_x_linear(float phase, float x)
{
  float x_bounded = skew_min + x * skew_range;
  return phase < x_bounded ? phase / x_bounded * 0.5f : 0.5f + (phase - x_bounded) / (1 - x_bounded) * 0.5f;
}

void
lfo_engine::reset(plugin_block const* block) 
{ 
  _ended = false;
  _end_value = 0; 
  _ref_phase = 0;
  _phase = block->state.own_block_automation[param_phase][0].real();
}

void
lfo_engine::process(plugin_block& block)
{
  int mode = block.state.own_block_automation[param_mode][0].step();
  if (mode == mode_off)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, 0.0f);
    return;
  }

  bool one_shot_full = mode == mode_rate_one || mode == mode_sync_one;
  bool one_shot_wrapped = mode == mode_rate_wrap || mode == mode_sync_wrap;
  if((one_shot_full || one_shot_wrapped) && _ended)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _end_value);
    return; 
  }

  int this_module = _global ? module_glfo : module_vlfo;
  int type = block.state.own_block_automation[param_type][0].step();
  bool sync = mode == mode_sync || mode == mode_sync_wrap || mode == mode_sync_one;
  auto const& x_curve = block.state.own_accurate_automation[param_x][0];
  auto const& y_curve = block.state.own_accurate_automation[param_y][0];
  auto const& rate_curve = sync_or_freq_into_scratch(block, sync, this_module, param_rate, param_tempo, scratch_time);

  //double log_half = std::log(0.5);
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    //double x_bounded = skew_min + x_curve[f] * skew_range;
    //double phase_skew = std::pow((double)_phase, std::log(x_bounded) / log_half);
    //double phase_skew2 = _phase < x_curve[f]? (_phase * 0.5f / x_curve[f]): 0.5f + 0.5f * (_phase - x_curve[f]) / (1 - x_curve[f]);
    switch (type)
    {
    case type_saw_plain: _end_value = skew_x_none(_phase, x_curve[f]); break;
    case type_sqr_plain: _end_value = skew_x_none(_phase, x_curve[f]) < 0.5f ? 0 : 1; break;
    case type_tri_plain: _end_value = 1 - std::fabs(unipolar_to_bipolar(skew_x_none(_phase, x_curve[f]))); break;
    case type_sin_plain: _end_value = bipolar_to_unipolar(std::sin(skew_x_none(_phase, x_curve[f]) * 2.0f * pi32)); break;
    case type_saw_lin: _end_value = skew_x_linear(_phase, x_curve[f]); break;
    case type_sqr_lin: _end_value = skew_x_linear(_phase, x_curve[f]) < 0.5f? 0: 1; break;
    case type_tri_lin: _end_value = 1 - std::fabs(unipolar_to_bipolar(skew_x_linear(_phase, x_curve[f]))); break;
    case type_sin_lin: _end_value = bipolar_to_unipolar(std::sin(skew_x_linear(_phase, x_curve[f]) * 2.0f * pi32)); break;
    case type_saw_log: _end_value = skew_x_log(_phase, x_curve[f]); break;
    case type_sqr_log: _end_value = skew_x_log(_phase, x_curve[f]) < 0.5f ? 0 : 1; break;
    case type_tri_log: _end_value = 1 - std::fabs(unipolar_to_bipolar(skew_x_log(_phase, x_curve[f]))); break;
    case type_sin_log: _end_value = bipolar_to_unipolar(std::sin(skew_x_log(_phase, x_curve[f]) * 2.0f * pi32)); break;
    default: break;

    //case type_saw: _end_value = phase_skew2; break;
    //case type_skew: _end_value = std::fabs(_phase - phase_skew); break;
    //case type_sqr: _end_value = _phase < x_bounded ? 0.0f : 1.0f; break;
    //case type_tri1: _end_value = 1 - std::fabs(unipolar_to_bipolar(phase_skew)); break;
    //case type_sine: _end_value = bipolar_to_unipolar(std::sin(2.0f * pi32 * phase_skew)); break;
    //case type_sine: _end_value = bipolar_to_unipolar(std::sin(2.0f * pi32 * phase_skew2)); break;
    //case type_tri2: _end_value = _phase < x_bounded ? _phase / x_bounded : 1 - (_phase - x_bounded) / (1 - x_bounded) ; break;
    }

    check_unipolar(_end_value);    
    block.state.own_cv[0][0][f] = skew_x_log(_end_value, y_curve[f]);
    bool phase_wrapped = increment_and_wrap_phase(_phase, rate_curve[f], block.sample_rate);
    bool ref_wrapped = increment_and_wrap_phase(_ref_phase, rate_curve[f], block.sample_rate);
    if((phase_wrapped && one_shot_wrapped) || (ref_wrapped && one_shot_full))
    {
      _ended = true;
      block.state.own_cv[0][0].fill(f + 1, block.end_frame, _end_value);
      return;
    }
  }
}

}
