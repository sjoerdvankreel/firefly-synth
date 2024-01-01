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
enum { type_sin, type_sin_log, type_saw, type_saw_lin, type_saw_log, type_pulse, type_pulse_lin, type_pulse_log };
//enum { type_sine, type_saw, type_sqr, type_skew, type_tri1, type_tri2, type_rnd_y, type_rnd_xy, type_rnd_y_free, type_rnd_xy_free };

#if 0
enum { 
  type_sin_x_pln_y_pln, type_sin_x_pln_y_lin, type_sin_x_pln_y_log, 
  type_sin_x_lin_y_pln, type_sin_x_lin_y_lin, type_sin_x_lin_y_log,
  type_sin_x_log_y_pln, type_sin_x_log_y_lin, type_sin_x_log_y_log,
  type_saw_x_pln_y_pln, type_saw_x_pln_y_lin, type_saw_x_pln_y_log,
  type_saw_x_lin_y_pln, type_saw_x_lin_y_lin, type_saw_x_lin_y_log,
  type_saw_x_log_y_pln, type_saw_x_log_y_lin, type_saw_x_log_y_log,
  type_sqr_x_pln_y_pln, type_sqr_x_pln_y_lin, type_sqr_x_pln_y_log,
  type_sqr_x_lin_y_pln, type_sqr_x_lin_y_lin, type_sqr_x_lin_y_log,
  type_sqr_x_log_y_pln, type_sqr_x_log_y_lin, type_sqr_x_log_y_log,
  type_tri_x_pln_y_pln, type_tri_x_pln_y_lin, type_tri_x_pln_y_log,
  type_tri_x_lin_y_pln, type_tri_x_lin_y_lin, type_tri_x_lin_y_log,
  type_tri_x_log_y_pln, type_tri_x_log_y_lin, type_tri_x_log_y_log };
#endif


static bool is_random(int type) { return false; }
static bool is_one_shot_full(int mode) { return mode == mode_rate_one || mode == mode_sync_one; }
static bool is_one_shot_wrapped(int mode) { return mode == mode_rate_wrap || mode == mode_sync_wrap; }
static bool is_sync(int mode) { return mode == mode_sync || mode == mode_sync_one || mode == mode_sync_wrap; }
static bool has_x(int type) { return type == type_sin_log || type == type_saw_lin || type == type_pulse_lin || type == type_pulse_log; }
static bool has_y(int type) { return type == type_sin_log || type == type_saw_lin || type == type_saw_log || type == type_pulse_lin || type == type_pulse_log; }

#if 0
static bool is_sin(int type) { return type_sin_x_pln_y_pln <= type && type <= type_sin_x_log_y_log; };
static bool is_saw(int type) { return type_saw_x_pln_y_pln <= type && type <= type_saw_x_log_y_log; };
static bool is_sqr(int type) { return type_sqr_x_pln_y_pln <= type && type <= type_sqr_x_log_y_log; };
static bool is_tri(int type) { return type_tri_x_pln_y_pln <= type && type <= type_tri_x_log_y_log; };

static bool is_x_lin(int type) { return
  type == type_sin_x_lin_y_pln || type == type_sin_x_lin_y_lin || type == type_sin_x_lin_y_log ||
  type == type_saw_x_lin_y_pln || type == type_saw_x_lin_y_lin || type == type_saw_x_lin_y_log ||
  type == type_sqr_x_lin_y_pln || type == type_sqr_x_lin_y_lin || type == type_sqr_x_lin_y_log ||
  type == type_tri_x_lin_y_pln || type == type_tri_x_lin_y_lin || type == type_tri_x_lin_y_log; }
static bool is_x_log(int type) { return
  type == type_sin_x_log_y_pln || type == type_sin_x_log_y_lin || type == type_sin_x_log_y_log ||
  type == type_saw_x_log_y_pln || type == type_saw_x_log_y_lin || type == type_saw_x_log_y_log ||
  type == type_sqr_x_log_y_pln || type == type_sqr_x_log_y_lin || type == type_sqr_x_log_y_log ||
  type == type_tri_x_log_y_pln || type == type_tri_x_log_y_lin || type == type_tri_x_log_y_log; }
static bool is_x_plain(int type) { return
  type == type_sin_x_pln_y_pln || type == type_sin_x_pln_y_lin || type == type_sin_x_pln_y_log ||
  type == type_saw_x_pln_y_pln || type == type_saw_x_pln_y_lin || type == type_saw_x_pln_y_log ||
  type == type_sqr_x_pln_y_pln || type == type_sqr_x_pln_y_lin || type == type_sqr_x_pln_y_log ||
  type == type_tri_x_pln_y_pln || type == type_tri_x_pln_y_lin || type == type_tri_x_pln_y_log; }

static bool is_y_lin(int type) { return
  type == type_sin_x_pln_y_lin || type == type_sin_x_lin_y_lin || type == type_sin_x_log_y_lin ||
  type == type_saw_x_pln_y_lin || type == type_saw_x_lin_y_lin || type == type_saw_x_log_y_lin ||
  type == type_sqr_x_pln_y_lin || type == type_sqr_x_lin_y_lin || type == type_sqr_x_log_y_lin ||
  type == type_tri_x_pln_y_lin || type == type_tri_x_lin_y_lin || type == type_tri_x_log_y_lin; }
static bool is_y_log(int type) { return
  type == type_sin_x_pln_y_log || type == type_sin_x_lin_y_log || type == type_sin_x_log_y_log ||
  type == type_saw_x_pln_y_log || type == type_saw_x_lin_y_log || type == type_saw_x_log_y_log ||
  type == type_sqr_x_pln_y_log || type == type_sqr_x_lin_y_log || type == type_sqr_x_log_y_log ||
  type == type_tri_x_pln_y_log || type == type_tri_x_lin_y_log || type == type_tri_x_log_y_log; }
static bool is_y_plain(int type) { return
  type == type_sin_x_pln_y_pln || type == type_sin_x_lin_y_pln || type == type_sin_x_log_y_pln ||
  type == type_saw_x_pln_y_pln || type == type_saw_x_lin_y_pln || type == type_saw_x_log_y_pln ||
  type == type_sqr_x_pln_y_pln || type == type_sqr_x_lin_y_pln || type == type_sqr_x_log_y_pln ||
  type == type_tri_x_pln_y_pln || type == type_tri_x_lin_y_pln || type == type_tri_x_log_y_pln; }

#endif

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{3B223DEC-1085-4D44-9C16-05B7FAA22006}", "Sin");
  result.emplace_back("{1EFB2A08-9E19-4BDB-B605-FAA7DAF3E154}", "Sin.Log");
  result.emplace_back("{2190619A-CB71-47F3-9B93-364BF4DA6BE6}", "Saw");
  result.emplace_back("{C304D3F4-3D77-437D-BFBD-4BBDA2FC90A5}", "Saw.Lin");
  result.emplace_back("{C91E269F-E83D-41A6-8C64-C34DBF9144C1}", "Saw.Log");
  result.emplace_back("{34480144-5349-49C1-9211-4CED6E6C8203}", "Pulse");
  result.emplace_back("{91CB7634-9759-485A-9DFF-6F5F86966212}", "Pulse.Lin");
  result.emplace_back("{D7C4F77F-E49E-4571-9A44-AD2AAE8F4C3E}", "Pulse.Log");
#if 0
  result.emplace_back("{1EFB2A08-9E19-4BDB-B605-FAA7DAF3E154}", "Sin.XPln/YLin");
  result.emplace_back("{125FB2CC-95EB-499E-97CE-469C72B37D73}", "Sin.XPln/YLog");
  result.emplace_back("{83416F5F-726A-4DC2-908F-181FEC32665F}", "Sin.XLin/YPln");
  result.emplace_back("{26C1E305-7CD0-443B-A474-6CFA91620348}", "Sin.XLin/YLin");
  result.emplace_back("{DB007351-F9D0-4CC9-A6A7-61179CA4B05D}", "Sin.XLin/YLog");
  result.emplace_back("{97C16D10-0850-4957-BB9A-C69AEEC7EA3C}", "Sin.XLog/YPln");
  result.emplace_back("{19354C8F-8527-4C5F-A131-9DB2804D0683}", "Sin.XLog/YLin");
  result.emplace_back("{9B28E73F-BE52-4463-9202-4B569505CE82}", "Sin.XLog/YLog");
  result.emplace_back("{06B82D77-D78A-4B39-9DB1-C921C36EF4D1}", "Saw.XPln/YPln");
  result.emplace_back("{A49D1C16-B20F-4E48-99A2-EB874B50B99C}", "Saw.XPln/YLin");
  result.emplace_back("{361F0904-E6F1-4E4E-B756-9E55B73700CD}", "Saw.XPln/YLog");
  result.emplace_back("{28C8EE94-20DE-4ECE-AEBD-657C28435BFA}", "Saw.XLin/YPln");
  result.emplace_back("{D592F9C5-6078-43DA-98BB-FC1ACF43112B}", "Saw.XLin/YLin");
  result.emplace_back("{EB264A9D-AC76-487C-B62A-5A45BFC4DD26}", "Saw.XLin/YLog");
  result.emplace_back("{32276066-B2D1-4712-8F99-59B6F91E6E67}", "Saw.XLog/YPln");
  result.emplace_back("{0489CEF2-1B9B-4C78-AEF3-FEBB3C4EEAC1}", "Saw.XLog/YLin");
  result.emplace_back("{539C72F6-D497-4947-A91F-84A705496D0F}", "Saw.XLog/YLog");
  result.emplace_back("{F396D88F-FF8B-4790-9173-7F90A3847072}", "Sqr.XPln/YPln");
  result.emplace_back("{4B5A2D22-968C-4190-8778-3483E08FA5F1}", "Sqr.XPln/YLin");
  result.emplace_back("{0AC9078E-4B23-4B2B-951C-9DE47C96F514}", "Sqr.XPln/YLog");
  result.emplace_back("{EC11B402-6DD8-442A-A5B4-D236E0691F18}", "Sqr.XLin/YPln");
  result.emplace_back("{9857C44E-E2F7-465E-90DC-5119CAC1C45E}", "Sqr.XLin/YLin");
  result.emplace_back("{773E9D4D-ABA2-44DF-82AD-166EFADF8463}", "Sqr.XLin/YLog");
  result.emplace_back("{A77B8F09-FEA2-4488-A96F-299666834987}", "Sqr.XLog/YPln");
  result.emplace_back("{8E4665F6-8507-4BFB-93CD-02EF577F6FA1}", "Sqr.XLog/YLin");
  result.emplace_back("{80FE0AEC-5568-4047-8505-27EE6EF1061C}", "Sqr.XLog/YLog");
  result.emplace_back("{428C8D6A-32D1-40EC-AC85-F0DB18053507}", "Tri.XPln/YPln");
  result.emplace_back("{DE9C5AB1-2CE9-4566-A0DA-70090FA82AD7}", "Tri.XPln/YLin");
  result.emplace_back("{C3411319-4B6C-4962-81D6-B20F160F8E03}", "Tri.XPln/YLog");
  result.emplace_back("{84B08C5F-CB0D-4F21-8EB6-E1D2830A7EC4}", "Tri.XLin/YPln");
  result.emplace_back("{770364D6-EC0D-4CB3-8736-F8B48B0E070E}", "Tri.XLin/YLin");
  result.emplace_back("{3AA84041-D517-48D0-9870-48D6C77FB613}", "Tri.XLin/YLog");
  result.emplace_back("{E89FA20C-81E0-4DBB-93DC-06F0D5BAB0C5}", "Tri.XLog/YPln");
  result.emplace_back("{9D282F75-6517-4291-AE7F-DE65DC0FB582}", "Tri.XLog/YLin");
  result.emplace_back("{90C08110-BDD3-4343-9B81-60ADD7FE7D3B}", "Tri.XLog/YLog");
#endif

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
  lfo_engine(bool global) : _global(global) {}

  void process(plugin_block& block) override;
  template <class Calc> void process_loop(plugin_block& block, Calc calc);
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
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 3, 3, 4, 4 }))));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Type", param_type, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(type_items(), ""),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  type.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
#if 0
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->add_submenu("Sin", { type_sin_x_pln_y_pln, type_sin_x_pln_y_lin, type_sin_x_pln_y_log,
    type_sin_x_lin_y_pln, type_sin_x_lin_y_lin, type_sin_x_lin_y_log,
    type_sin_x_log_y_pln, type_sin_x_log_y_lin, type_sin_x_log_y_log });
  type.gui.submenu->add_submenu("Saw", { type_saw_x_pln_y_pln, type_saw_x_pln_y_lin, type_saw_x_pln_y_log,
    type_saw_x_lin_y_pln, type_saw_x_lin_y_lin, type_saw_x_lin_y_log,
    type_saw_x_log_y_pln, type_saw_x_log_y_lin, type_saw_x_log_y_log });
  type.gui.submenu->add_submenu("Sqr", { type_sqr_x_pln_y_pln, type_sqr_x_pln_y_lin, type_sqr_x_pln_y_log,
    type_sqr_x_lin_y_pln, type_sqr_x_lin_y_lin, type_sqr_x_lin_y_log,
    type_sqr_x_log_y_pln, type_sqr_x_log_y_lin, type_sqr_x_log_y_log });
  type.gui.submenu->add_submenu("Tri", {type_tri_x_pln_y_pln, type_tri_x_pln_y_lin, type_tri_x_pln_y_log,
    type_tri_x_lin_y_pln, type_tri_x_lin_y_lin, type_tri_x_lin_y_log,
    type_tri_x_log_y_pln, type_tri_x_log_y_lin, type_tri_x_log_y_log });
  // TODO type.gui.submenu->add_submenu("Random", { type_rnd_y, type_rnd_xy, type_rnd_y_free, type_rnd_xy_free });
#endif

  auto& x = result.params.emplace_back(make_param(
    make_topo_info("{8CEDE705-8901-4247-9854-83FB7BEB14F9}", "X", "X", true, param_x, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 1 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  x.gui.bindings.enabled.bind_params({ param_mode, param_type }, [](auto const& vs) { return vs[0] != mode_off && has_x(vs[1]); });
  auto& y = result.params.emplace_back(make_param(
    make_topo_info("{8939B05F-8677-4AA9-8C4C-E6D96D9AB640}", "Y", "Y", true, param_y, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 2 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  y.gui.bindings.enabled.bind_params({ param_mode, param_type }, [](auto const& vs) { return vs[0] != mode_off && has_y(vs[1]); });
  auto& smooth = result.params.emplace_back(make_param(
    make_topo_info("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", "Smt", true, param_filter, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 3 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
  smooth.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  auto& phase = result.params.emplace_back(make_param(
    make_topo_info("{B23E9732-ECE3-4D5D-8EC1-FF299C6926BB}", "Phase", "Phs", true, param_phase, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::short_name, gui_label_align::left, gui_label_justify::center))));
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

#if 0
static float calc_saw(float in) { return in; }
static float calc_sqr(float in) { return in < 0.5f ? 0 : 1; }
static float calc_tri(float in) { return 1 - std::fabs(unipolar_to_bipolar(in)); }
static float calc_sin(float in) { return bipolar_to_unipolar(std::sin(in * 2.0f * pi32)); }

static float
skew_none(float in, float skew)
{ 
  return in;
} 

static float
skew_log(float in, float skew)
{
  float skew_bounded = skew_min + skew * skew_range;
  return std::pow(in, std::log(skew_bounded) / log_half);
}

static float
skew_lin(float in, float skew)
{
  float skew_bounded = skew_min + skew * skew_range;
  return in < skew_bounded ? in / skew_bounded * 0.5f : 0.5f + (in - skew_bounded) / (1 - skew_bounded) * 0.5f;
}
#endif

static float
skew_log(float in, float skew)
{ return std::pow(in, std::log(0.001 + (skew * 0.999)) / log_half); }
static float
calc_saw(float phase, float x, float y)
{ return phase; }
static float
calc_sin(float phase, float x, float y)
{ return bipolar_to_unipolar(std::sin(phase * 2.0f * pi32)); }
static float
calc_sin_log(float phase, float x, float y)
{ return skew_log(bipolar_to_unipolar(std::sin(skew_log(phase, x) * 2.0f * pi32)), y); }
static float
calc_saw_lin(float phase, float x, float y)
{ return phase < x? phase * y / x : y +  (phase - x) / ( 1 - x) * (1 - y); }
static float
calc_saw_log(float phase, float x, float y)
{ return skew_log(calc_saw(phase, x, y), y); }
static float
calc_pulse(float phase, float x, float y)
{ return phase < 0.5? 0: 1; }
static float
calc_pulse_lin(float phase, float x, float y)
{ return phase < x? 0: y; }
static float
calc_pulse_log(float phase, float x, float y)
{ return bipolar_to_unipolar((calc_saw(phase, x, y) - calc_saw_log(phase + x - std::floor(phase + x), x, y)) * 2.0f); }

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

  if((is_one_shot_full(mode) || is_one_shot_wrapped(mode)) && _ended)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _end_value);
    return; 
  }

  int type = block.state.own_block_automation[param_type][0].step();
  switch (type)
  {
  case type_sin: process_loop(block, calc_sin); break;
  case type_sin_log: process_loop(block, calc_sin_log); break;
  case type_saw: process_loop(block, calc_saw); break;
  case type_saw_lin: process_loop(block, calc_saw_lin); break;
  case type_saw_log: process_loop(block, calc_saw_log); break;
  case type_pulse: process_loop(block, calc_pulse); break;
  case type_pulse_lin: process_loop(block, calc_pulse_lin); break;
  case type_pulse_log: process_loop(block, calc_pulse_log); break;
  default: assert(false); break;
  }

#if 0
  if(is_sin(type))
  {
    if(is_x_plain(type))
    {
      if(is_y_plain(type)) process_loop(block, calc_sin, skew_none, skew_none);
      else if(is_y_lin(type)) process_loop(block, calc_sin, skew_none, skew_lin);
      else if(is_y_log(type)) process_loop(block, calc_sin, skew_none, skew_log);
      else assert(false);
    }
    else if (is_x_lin(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_sin, skew_lin, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_sin, skew_lin, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_sin, skew_lin, skew_log);
      else assert(false);
    }
    else if (is_x_log(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_sin, skew_log, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_sin, skew_log, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_sin, skew_log, skew_log);
      else assert(false);
    } else assert(false);
  } else if (is_saw(type))
  {
    if (is_x_plain(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_saw, skew_none, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_saw, skew_none, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_saw, skew_none, skew_log);
      else assert(false);
    }
    else if (is_x_lin(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_saw, skew_lin, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_saw, skew_lin, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_saw, skew_lin, skew_log);
      else assert(false);
    }
    else if (is_x_log(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_saw, skew_log, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_saw, skew_log, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_saw, skew_log, skew_log);
      else assert(false);
    }
    else assert(false);
  } else if (is_sqr(type))
  {
    if (is_x_plain(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_sqr, skew_none, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_sqr, skew_none, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_sqr, skew_none, skew_log);
      else assert(false);
    }
    else if (is_x_lin(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_sqr, skew_lin, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_sqr, skew_lin, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_sqr, skew_lin, skew_log);
      else assert(false);
    }
    else if (is_x_log(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_sqr, skew_log, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_sqr, skew_log, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_sqr, skew_log, skew_log);
      else assert(false);
    }
  }
  else if (is_tri(type))
  {
    if (is_x_plain(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_tri, skew_none, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_tri, skew_none, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_tri, skew_none, skew_log);
      else assert(false);
    }
    else if (is_x_lin(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_tri, skew_lin, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_tri, skew_lin, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_tri, skew_lin, skew_log);
      else assert(false);
    }
    else if (is_x_log(type))
    {
      if (is_y_plain(type)) process_loop(block, calc_tri, skew_log, skew_none);
      else if (is_y_lin(type)) process_loop(block, calc_tri, skew_log, skew_lin);
      else if (is_y_log(type)) process_loop(block, calc_tri, skew_log, skew_log);
      else assert(false);
    }
  } else
    assert(false);
#endif
}

template <class Calc>
void lfo_engine::process_loop(plugin_block& block, Calc calc)
{
  int this_module = _global ? module_glfo : module_vlfo;
  int mode = block.state.own_block_automation[param_mode][0].step();
  auto const& x_curve = block.state.own_accurate_automation[param_x][0];
  auto const& y_curve = block.state.own_accurate_automation[param_y][0];
  auto const& rate_curve = sync_or_freq_into_scratch(block, is_sync(mode), this_module, param_rate, param_tempo, scratch_time);

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    //_end_value = skew_y(calc(skew_x(_phase, x_curve[f])), y_curve[f]);
    _end_value = calc(_phase, x_curve[f], y_curve[f]);
    block.state.own_cv[0][0][f] = check_unipolar(_end_value);
    bool phase_wrapped = increment_and_wrap_phase(_phase, rate_curve[f], block.sample_rate);
    bool ref_wrapped = increment_and_wrap_phase(_ref_phase, rate_curve[f], block.sample_rate);
    if((phase_wrapped && is_one_shot_wrapped(mode)) || (ref_wrapped && is_one_shot_full(mode)))
    {
      _ended = true;
      block.state.own_cv[0][0].fill(f + 1, block.end_frame, _end_value);
      return;
    }
  }
}

}
