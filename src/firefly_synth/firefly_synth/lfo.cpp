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

enum { section_mode, section_type };
enum { scratch_time, scratch_count };
enum { mode_off, mode_rate, mode_rate_one, mode_sync, mode_sync_one };
enum { type_sine, type_saw, type_sqr, type_tri, type_rnd_y, type_rnd_xy, type_rnd_y_free, type_rnd_xy_free };
enum { param_mode, param_rate, param_tempo, param_type, param_x, param_y, param_smooth, param_phase, param_seed };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{DE8FF99D-C83F-4723-B8DA-FB1C4877B1F4}", "Sine");
  result.emplace_back("{01636F45-4734-4762-B475-E4CA15BAE156}", "Saw");
  result.emplace_back("{497E9796-48D2-4C33-B502-0C3AE3FD03D1}", "Sqr");
  result.emplace_back("{0B88AFD3-C8F3-4FA1-93D8-D2D074D5F6A7}", "Tri");
  result.emplace_back("{83EF2C08-E5A1-4517-AC8C-D45890936A96}", "Rnd.Y");
  result.emplace_back("{84BFAC67-D748-4499-813F-0B7FCEBF174B}", "Rnd.XY");
  result.emplace_back("{F6B72990-D053-4D3A-9B8D-391DDB748DC1}", "RndF.Y");
  result.emplace_back("{7EBDA9FE-11D9-4C09-BA4B-EF3763FB3CF0}", "RndF.XY");
  return result;
}

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{E8D04800-17A9-42AB-9CAE-19322A400334}", "Off");
  result.emplace_back("{5F57863F-4157-4F53-BB02-C6693675B881}", "Rate");
  result.emplace_back("{12E9AF37-1C1F-43AB-9405-86F103293C4C}", "Rate.One");
  result.emplace_back("{E2692483-F48B-4037-BF74-64BB62110538}", "Sync");
  result.emplace_back("{9CFBC6ED-1024-4FDE-9291-9280FDA9BC1E}", "Sync.One");
  return result;
}

class lfo_engine :
public module_engine {
  bool _ended;
  float _phase;
  float _end_value;
  bool const _global;
public:
  PB_PREVENT_ACCIDENTAL_COPY(lfo_engine);
  lfo_engine(bool global) : _global(global) {}
  void process(plugin_block& block) override;
  void reset(plugin_block const*) override { _phase = 0; _end_value = 0; _ended = false; }
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
render_graph(plugin_state const& state, graph_engine* engine, param_topo_mapping const& mapping)
{
  if(state.get_plain_at(mapping.module_index, mapping.module_slot, param_mode, 0).step() == mode_off)
    return graph_data(graph_data_type::off, {});
  auto const params = make_graph_engine_params();
  int sample_rate = params.max_frame_count;
  
  // draw synced 1/4 as full cycle
  int mode = state.get_plain_at(mapping.module_index, mapping.module_slot, param_mode, mapping.param_slot).step();
  if (mode == mode_sync || mode == mode_sync_one)
  {
    float one_beat_freq = timesig_to_freq(120, { 1, 4 });
    sample_rate = one_beat_freq * params.max_frame_count;
  }

  engine->process_begin(&state, sample_rate, params.max_frame_count, -1);
  auto const* block = engine->process_default(mapping.module_index, mapping.module_slot);
  engine->process_end();
  jarray<float, 1> series(block->state.own_cv[0][0]);
  series.insert(series.begin(), 0.0f);
  series.push_back(0.0f);
  return graph_data(series, false, {});
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
  result.params.emplace_back(make_param(
    make_topo_info("{252D76F2-8B36-4F15-94D0-2E974EC64522}", "Mode", param_mode, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(mode_items(), ""),
    make_param_gui_single(section_mode, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  auto& rate = result.params.emplace_back(make_param(
    make_topo_info("{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(0.1, 20, 1, 2, "Hz"),
    make_param_gui_single(section_mode, gui_edit_type::knob, { 0, 1 }, gui_label_contents::none,
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));
  rate.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  rate.gui.bindings.visible.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_sync && vs[0] != mode_sync_one; });
  auto& tempo = result.params.emplace_back(make_param(
    make_topo_info("{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Tempo", param_tempo, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_timesig_default(false, { 1, 4 }),
    make_param_gui_single(section_mode, gui_edit_type::list, { 0, 1 }, gui_label_contents::name, make_label_none())));
  tempo.gui.submenu = make_timesig_submenu(tempo.domain.timesigs);
  tempo.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  tempo.gui.bindings.visible.bind_params({ param_mode }, [](auto const& vs) { return vs[0] == mode_sync || vs[0] == mode_sync_one; });
  
  result.sections.emplace_back(make_param_section(section_type,
    make_topo_tag("{A5B5DC53-2E73-4C0B-9DD1-721A335EA076}", "Type"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 3, 3, 5, 5 }))));
  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{7D48C09B-AC99-4B88-B880-4633BC8DFB37}", "Type", param_type, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(type_items(), ""),
    make_param_gui_single(section_type, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  type.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  type.gui.submenu = std::make_shared<gui_submenu>();
  auto classic_menu = std::make_shared<gui_submenu>();
  classic_menu->name = "Classic";
  classic_menu->indices.push_back(type_sine);
  classic_menu->indices.push_back(type_saw);
  classic_menu->indices.push_back(type_sqr);
  classic_menu->indices.push_back(type_tri);
  type.gui.submenu->children.push_back(classic_menu);
  auto random_menu = std::make_shared<gui_submenu>();
  random_menu->name = "Random";
  random_menu->indices.push_back(type_rnd_y);
  random_menu->indices.push_back(type_rnd_xy);
  random_menu->indices.push_back(type_rnd_y_free);
  random_menu->indices.push_back(type_rnd_xy_free);
  type.gui.submenu->children.push_back(random_menu);

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
    make_topo_info("{21DBFFBE-79DA-45D4-B778-AC939B7EF785}", "Smooth", "Smth", true, param_smooth, 1),
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
  phase.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] < type_rnd_y; });
  auto& seed = result.params.emplace_back(make_param(
    make_topo_info("{9F5BE73B-20C0-44C5-B078-CD571497A837}", "Seed", param_seed, 1),
    make_param_dsp_input(!global, param_automate::none), make_domain_step(1, 255, 1, 0),
    make_param_gui_single(section_type, gui_edit_type::knob, { 0, 4 }, gui_label_contents::value,
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  seed.gui.submenu = make_midi_note_submenu();
  seed.gui.label_reference_text = phase.info.tag.name;
  seed.gui.bindings.enabled.bind_params({ param_mode }, [](auto const& vs) { return vs[0] != mode_off; });
  seed.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] >= type_rnd_y; });

  return result;
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

  bool one_shot = mode == mode_rate_one || mode == mode_sync_one;
  if(one_shot && _ended)
  {
    block.state.own_cv[0][0].fill(block.start_frame, block.end_frame, _end_value);
    return; 
  }

  int this_module = _global ? module_glfo : module_vlfo;
  bool sync = mode == mode_sync || mode == mode_sync_one;
  int type = block.state.own_block_automation[param_type][0].step();
  auto const& rate_curve = sync_or_freq_into_scratch(
    block, sync, this_module, param_rate, param_tempo, scratch_time);
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    switch (type)
    {
    case type_saw: _end_value = _phase; break;
    case type_sqr: _end_value = _phase < 0.5f? 0.0f: 1.0f; break;
    case type_tri: _end_value = 1 - std::fabs(unipolar_to_bipolar(_phase)); break;
    case type_sine: _end_value = bipolar_to_unipolar(std::sin(2.0f * pi32 * _phase)); break;
    } 
    
    block.state.own_cv[0][0][f] = _end_value;
    if(increment_and_wrap_phase(_phase, rate_curve[f], block.sample_rate) && one_shot)
    {
      _ended = true;
      block.state.own_cv[0][0].fill(f + 1, block.end_frame, _end_value);
      return;
    }
  }
}

}
