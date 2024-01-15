#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base; 

namespace firefly_synth {

static int constexpr voice_route_count = 30;
static int constexpr global_route_count = 20;
static int constexpr max_route_count = std::max(voice_route_count, global_route_count);

enum { section_main };
enum { param_type, param_source, param_target, param_min, param_max };
enum { type_off, type_mul_abs, type_mul_rel, type_mul_stk, type_add_abs, type_add_rel, type_add_stk, type_ab_abs, type_ab_rel, type_ab_stk };

extern void
env_plot_length_seconds(
  plugin_state const& state, int slot, float& dahds, float& dahdsrf);
extern float
lfo_frequency_from_state(
  plugin_state const& state, int module_index, int module_slot, int bpm);

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{7CE8B8A1-0711-4BDE-BDFF-0F97BF16EB57}", "Off");
  result.emplace_back("{C185C0A7-AE6A-4ADE-8171-119A96C24233}", "Mul.Abs");
  result.emplace_back("{51FBF610-2039-448E-96B0-3C5FDED5DC7C}", "Mul.Rel");
  result.emplace_back("{D9AEAC34-8E75-4EFD-91D3-6F9058226816}", "Mul.Stk");
  result.emplace_back("{000C0860-B191-4554-9249-85846B1AFFD1}", "Add.Abs");
  result.emplace_back("{169406D2-E86F-4275-A49F-59ED67CD7661}", "Add.Rel");
  result.emplace_back("{621467B6-CFB7-4801-9DF4-6F9A200AD098}", "Add.Stk");
  result.emplace_back("{23FB17DA-B98B-49FF-8D46-4E5FE7F486D6}", "AB.Abs");
  result.emplace_back("{6708DDD1-14EA-4E1D-8A1F-E4FFE76A87F0}", "AB.Rel");
  result.emplace_back("{1CCAB37F-0AA7-4A77-8C4C-28838970665B}", "AB.Stk");
  return result;
}

class cv_matrix_engine:
public module_engine { 
  bool const _global;
  cv_matrix_mixdown _mixdown = {};
  jarray<int, 4> _modulation_indices = {};
  std::vector<param_topo_mapping> const _targets;
  std::vector<module_output_mapping> const _sources;
public:
  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override;

  cv_matrix_engine(
    bool global,
    plugin_topo const& topo, 
    std::vector<module_output_mapping> const& sources,
    std::vector<param_topo_mapping> const& targets);
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_matrix_engine);
};

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vcv_matrix, 0, param_type, 0, "Add.Abs");
  state.set_text_at(module_vcv_matrix, 0, param_source, 0, "Env 2");
  state.set_text_at(module_vcv_matrix, 0, param_target, 0, "V.FX 1 SVF.Frq");
  state.set_text_at(module_vcv_matrix, 0, param_type, 1, "AB.Abs");
  state.set_text_at(module_vcv_matrix, 0, param_min, 1, "35");
  state.set_text_at(module_vcv_matrix, 0, param_max, 1, "65");
  state.set_text_at(module_vcv_matrix, 0, param_source, 1, "G.LFO 2");
  state.set_text_at(module_vcv_matrix, 0, param_target, 1, "V.Audio Bal 1");
  state.set_text_at(module_vcv_matrix, 0, param_type, 2, "AB.Abs");
  state.set_text_at(module_vcv_matrix, 0, param_source, 2, "M.In PB");
  state.set_text_at(module_vcv_matrix, 0, param_target, 2, "V.In PB");
  state.set_text_at(module_vcv_matrix, 0, param_type, 3, "Mul.Abs");
  state.set_text_at(module_vcv_matrix, 0, param_source, 3, "Note Velo");
  state.set_text_at(module_vcv_matrix, 0, param_target, 3, "V.Out Gain");
}

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_gcv_matrix, 0, param_type, 0, "AB.Abs");
  state.set_text_at(module_gcv_matrix, 0, param_min, 0, "35");
  state.set_text_at(module_gcv_matrix, 0, param_max, 0, "65");
  state.set_text_at(module_gcv_matrix, 0, param_source, 0, "G.LFO 1");
  state.set_text_at(module_gcv_matrix, 0, param_target, 0, "G.FX 1 SVF.Frq");
  state.set_text_at(module_gcv_matrix, 0, param_type, 1, "Add.Abs");
  state.set_text_at(module_gcv_matrix, 0, param_source, 1, "M.In Mod");
  state.set_text_at(module_gcv_matrix, 0, param_target, 1, "G.FX 1 SVF.Frq");
}

audio_routing_cv_params
make_audio_routing_cv_params(plugin_state* state, bool global)
{
  audio_routing_cv_params result;
  result.off_value = type_off;
  result.on_param = param_type;
  result.target_param = param_target;
  result.matrix_module = global ? module_gcv_matrix : module_vcv_matrix;
  result.targets = make_cv_target_matrix(make_cv_matrix_targets(state->desc().plugin, global)).mappings;
  return result;
}

std::unique_ptr<module_tab_menu_handler>
make_cv_routing_menu_handler(plugin_state* state)
{
  std::map<int, std::vector<module_output_mapping>> matrix_sources;
  matrix_sources[module_gcv_matrix] = make_cv_source_matrix(make_cv_matrix_sources(state->desc().plugin, true)).mappings;
  matrix_sources[module_vcv_matrix] = make_cv_source_matrix(make_cv_matrix_sources(state->desc().plugin, false)).mappings;
  return std::make_unique<cv_routing_menu_handler>(state, param_source, param_type, type_off, matrix_sources);
}

void
select_midi_active(
  plugin_state const& state, bool global, int on_note_midi_start,
  std::vector<module_output_mapping> const& mappings, jarray<int, 3>& active)
{
  int module = global ? module_gcv_matrix : module_vcv_matrix;
  // PB and mod wheel are linked so must be always on
  active[module_midi][0][midi_source_pb] = 1;
  active[module_midi][0][midi_source_cc + 1] = 1;

  int route_count = global? global_route_count: voice_route_count;
  for (int r = 0; r < route_count; r++)
  {
    int type = state.get_plain_at(module, 0, param_type, r).step();
    if (type != type_off)
    {
      int source = state.get_plain_at(module, 0, param_source, r).step();
      auto const& mapping = mappings[source];
      if (mapping.module_index == module_midi)
      {
        if(mapping.output_index == midi_output_pb)
          active[module_midi][0][midi_source_pb] = 1;
        else if (mapping.output_index == midi_output_cp)
          active[module_midi][0][midi_source_cp] = 1;
        else if (mapping.output_index == midi_output_cc)
          active[module_midi][0][midi_source_cc + mapping.output_slot] = 1;
        else
          assert(false);
      }
      else if (mapping.module_index == module_voice_on_note)
      {
        // on note flattens the list of all global cv sources, with midi at end
        if(mapping.output_index == on_note_midi_start + midi_output_pb)
          active[module_midi][0][midi_source_pb] = 1;
        else if (mapping.output_index == on_note_midi_start + midi_output_cp)
          active[module_midi][0][midi_source_cp] = 1;
        else if (mapping.output_index >= on_note_midi_start + midi_output_cc)
          active[module_midi][0][midi_source_cc + mapping.output_index - on_note_midi_start - midi_output_cc] = 1;
      }
    }
  }
}

static graph_engine_params
make_graph_engine_params()
{
  graph_engine_params result = {};
  result.bpm = 120;
  result.max_frame_count = 400;
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
  plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping, 
  std::vector<module_output_mapping> const& sources, routing_matrix<param_topo_mapping> const& targets)
{
  auto const& map = mapping;
  bool global = map.module_index == module_gcv_matrix;
  int route_count = global ? global_route_count : voice_route_count;
  int type = state.get_plain_at(map.module_index, map.module_slot, param_type, map.param_slot).step();
  if(type == type_off) 
  {
    // try to always paint something
    for(int r = 0; r < route_count; r++)
      if(state.get_plain_at(map.module_index, map.module_slot, param_type, r).step() != type_off)
        return render_graph(state, engine, -1, { map.module_index, map.module_slot, map.param_index, r }, sources, targets);
    return graph_data(graph_data_type::off, {});
  }

  // scale to longest env or lfo
  float dahds = 0.1f;
  float dahdsrf = 0.1f;
  float max_total = 0.1f;
  float max_dahds = 0.1f;
  float max_dahdsrf = 0.1f;
  int ti = state.get_plain_at(map.module_index, map.module_slot, param_target, map.param_slot).step();
  for(int r = 0; r < route_count; r++)
    if (state.get_plain_at(map.module_index, map.module_slot, param_type, r).step() != type_off)
      if (state.get_plain_at(map.module_index, map.module_slot, param_target, r).step() == ti)
      {
        int si = state.get_plain_at(map.module_index, map.module_slot, param_source, r).step();
        if (sources[si].module_index == module_env)
        {
          env_plot_length_seconds(state, sources[si].module_slot, dahds, dahdsrf);
          if (dahdsrf > max_dahdsrf)
          {
            max_dahds = dahds;
            max_dahdsrf = dahdsrf;
            max_total = dahdsrf;
          }
        }
        else if (sources[si].module_index == module_glfo || sources[si].module_index == module_vlfo)
        {
          float freq = lfo_frequency_from_state(state, sources[si].module_index, sources[si].module_slot, 120);
          max_total = std::max(max_total, 1 / freq);
        }
      }

  auto const params = make_graph_engine_params();
  int sample_rate = params.max_frame_count / max_total;
  int voice_release_at = max_dahds / max_dahdsrf * params.max_frame_count;
  engine->process_begin(&state, sample_rate, params.max_frame_count, voice_release_at);
  std::vector<int> relevant_modules({ module_master_in, module_glfo });
  if(map.module_index == module_vcv_matrix)
    relevant_modules.insert(relevant_modules.end(), { module_voice_on_note, module_vlfo, module_env });
  for(int m = 0; m < relevant_modules.size(); m++)
    for(int mi = 0; mi < state.desc().plugin->modules[relevant_modules[m]].info.slot_count; mi++)
      engine->process_default(relevant_modules[m], mi);
  auto* block = engine->process_default(map.module_index, map.module_slot);
  engine->process_end();

  std::string partition = float_to_string(max_total, 1) + " Sec " + targets.items[ti].name;
  auto const& modulation = get_cv_matrix_mixdown(*block, map.module_index == module_gcv_matrix);
  jarray<float, 1> stacked = jarray<float, 1>(*targets.mappings[ti].value_at(modulation));
  return graph_data(stacked, false, true, { partition });
}

module_topo
cv_matrix_topo(
  int section, gui_colors const& colors,
  gui_position const& pos, bool global,
  std::vector<cv_source_entry> const& sources,
  std::vector<cv_source_entry> const& on_note_sources,
  std::vector<module_topo const*> const& targets)
{
  int on_note_midi_start = -1;
  auto source_matrix = make_cv_source_matrix(sources);
  auto target_matrix = make_cv_target_matrix(targets);
  auto const voice_info = make_topo_info("{5F794E80-735C-43E8-B8EC-83910D118AF0}", "Voice CV", "V.CV", true, true, module_vcv_matrix, 1);
  auto const global_info = make_topo_info("{DB22D4C1-EDA5-45F6-AE9B-183CA6F4C28D}", "Global CV", "G.CV", true, true, module_gcv_matrix, 1);
  auto const info = topo_info(global? global_info: voice_info);
  module_stage stage = global ? module_stage::input : module_stage::voice;

  if (!global)
  {
    auto on_note_matrix(make_cv_source_matrix(on_note_sources).mappings);
    for (int m = 0; m < on_note_matrix.size(); m++)
      if (on_note_matrix[m].module_index == module_midi) { on_note_midi_start = m; break; }
    assert(on_note_midi_start != -1);
  }

  int route_count = global ? global_route_count : voice_route_count;
  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 0, {
      make_module_dsp_output(false, make_topo_info("{3AEE42C9-691E-484F-B913-55EB05CFBB02}", "Output", 0, route_count)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));
  
  result.gui.tabbed_name = result.info.tag.short_name;
  result.default_initializer = global ? init_global_default : init_voice_default;
  result.graph_engine_factory = make_graph_engine;
  result.graph_renderer = [sm = source_matrix.mappings, tm = target_matrix](
    auto const& state, auto* engine, int param, auto const& mapping) {
      return render_graph(state, engine, param, mapping, sm, tm);
  };
  result.gui.menu_handler_factory = [](plugin_state* state) {
    return std::make_unique<tidy_matrix_menu_handler>(
      state, param_type, type_off, std::vector<int>({ param_target, param_source })); 
  };
  result.midi_active_selector = [global, on_note_midi_start, sm = source_matrix.mappings](
    plugin_state const& state, int, jarray<int, 3>& active) { 
      select_midi_active(state, global, on_note_midi_start, sm, active); 
  };
  result.engine_factory = [global, sm = source_matrix.mappings, tm = target_matrix.mappings](
    auto const& topo, int, int) { 
      return std::make_unique<cv_matrix_engine>(global, topo, sm, tm); 
  };

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 3, 4, -35, -35 } })));
  main.gui.scroll_mode = gui_scroll_mode::vertical;
  
  auto& type = result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Op", param_type, route_count),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(type_items(), ""),
    make_param_gui(section_main, gui_edit_type::autofit_list, param_layout::vertical, { 0, 0 }, make_label_none())));
  type.gui.tabular = true;
  type.gui.menu_handler_factory = [route_count](plugin_state* state) { return make_matrix_param_menu_handler(state, route_count); };
  type.gui.submenu = std::make_shared<gui_submenu>();
  type.gui.submenu->indices.push_back(type_off);
  auto mul_menu = std::make_shared<gui_submenu>();
  mul_menu->name = "Mul";
  mul_menu->indices.push_back(type_mul_abs);
  mul_menu->indices.push_back(type_mul_rel);
  mul_menu->indices.push_back(type_mul_stk);
  type.gui.submenu->children.push_back(mul_menu);
  auto add_menu = std::make_shared<gui_submenu>();
  add_menu->name = "Add";
  add_menu->indices.push_back(type_add_abs);
  add_menu->indices.push_back(type_add_rel);
  add_menu->indices.push_back(type_add_stk);
  type.gui.submenu->children.push_back(add_menu);
  auto ab_menu = std::make_shared<gui_submenu>();
  ab_menu->name = "Add Bipolar";
  ab_menu->indices.push_back(type_ab_abs);
  ab_menu->indices.push_back(type_ab_rel);
  ab_menu->indices.push_back(type_ab_stk);
  type.gui.submenu->children.push_back(ab_menu);

  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(source_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.tabular = true;
  source.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  source.gui.submenu = source_matrix.submenu;

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_input(!global, param_automate::none), make_domain_item(target_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.tabular = true;
  target.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  target.gui.submenu = target_matrix.submenu;
  target.gui.item_enabled.auto_bind = true;

  auto& min = result.params.emplace_back(make_param(
    make_topo_info("{71E6F836-1950-4C8D-B62B-FAAD20B1FDBD}", "Min", param_min, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  min.gui.tabular = true;
  min.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  auto& max = result.params.emplace_back(make_param(
    make_topo_info("{DB3A5D43-95CB-48DC-97FA-984F55B57F7B}", "Max", param_max, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 4 }, make_label_none())));
  max.gui.tabular = true;
  max.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  return result;
}

cv_matrix_engine::
cv_matrix_engine(
  bool global,
  plugin_topo const& topo,
  std::vector<module_output_mapping> const& sources,
  std::vector<param_topo_mapping> const& targets):
_global(global), _sources(sources), _targets(targets)
{
  plugin_dims dims(topo, topo.audio_polyphony);
  _mixdown.resize(dims.module_slot_param_slot);
  _modulation_indices.resize(dims.module_slot_param_slot);
}

void
cv_matrix_engine::process(plugin_block& block)
{
  // set every modulatable parameter to its corresponding automation curve
  for (int m = 0; m < _targets.size(); m++)
  {
    int tp = _targets[m].param_index;
    int tpi = _targets[m].param_slot;
    int tm = _targets[m].module_index;
    int tmi = _targets[m].module_slot;
    _modulation_indices[tm][tmi][tp][tpi] = -1;
    auto const& curve = block.state.all_accurate_automation[tm][tmi][tp][tpi];
    _mixdown[tm][tmi][tp][tpi] = &curve;
  }

  // apply modulation routing
  int modulation_index = 0;
  auto const& own_automation = block.state.own_block_automation;
  int route_count = _global ? global_route_count : voice_route_count;
  jarray<float, 1>* modulated_curve_ptrs[max_route_count] = { nullptr };
  for (int r = 0; r < route_count; r++)
  {
    jarray<float, 1>* modulated_curve_ptr = nullptr;
    int type = own_automation[param_type][r].step();
    if(type == type_off) continue;

    // found out indices of modulation target
    int selected_target = own_automation[param_target][r].step();
    int tp = _targets[selected_target].param_index;
    int tpi = _targets[selected_target].param_slot;
    int tm = _targets[selected_target].module_index;
    int tmi = _targets[selected_target].module_slot;
    auto const& target_curve = block.state.all_accurate_automation[tm][tmi][tp][tpi];

    // if already modulated, set target curve to own buffer
    int existing_modulation_index = _modulation_indices[tm][tmi][tp][tpi];
    if (existing_modulation_index != -1)
      modulated_curve_ptr = &block.state.own_cv[0][existing_modulation_index];
    else
    {
      // else pick the next of our own cv outputs
      modulated_curve_ptr = &block.state.own_cv[0][modulation_index];
      auto const& target_automation = block.state.all_accurate_automation[tm][tmi][tp][tpi];
      target_automation.copy_to(block.start_frame, block.end_frame, *modulated_curve_ptr);
      modulated_curve_ptrs[r] = modulated_curve_ptr;
      _mixdown[tm][tmi][tp][tpi] = modulated_curve_ptr;
      _modulation_indices[tm][tmi][tp][tpi] = modulation_index++;
    }

    assert(modulated_curve_ptr != nullptr);
    jarray<float, 1>& modulated_curve = *modulated_curve_ptr;

    // find out indices of modulation source
    int selected_source = own_automation[param_source][r].step();
    int sm = _sources[selected_source].module_index;
    int smi = _sources[selected_source].module_slot;
    int so = _sources[selected_source].output_index;
    int soi = _sources[selected_source].output_slot;
    auto const& source_curve = block.module_cv(sm, smi)[so][soi];

    // source must be regular unipolar otherwise add/bi/mul breaks
    for(int f = block.start_frame; f < block.end_frame; f++)
      check_unipolar(source_curve[f]);

    // apply modulation
    auto const& min_curve = block.state.own_accurate_automation[param_min][r];
    auto const& max_curve = block.state.own_accurate_automation[param_max][r];
    switch (type)
    {
    case type_mul_abs:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] *= min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f];
      break;
    case type_mul_rel:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] = target_curve[f] + (min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]) * (1 - target_curve[f]);
      break;
    case type_mul_stk:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] = modulated_curve[f] + (min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]) * (1 - modulated_curve[f]);
      break;
    case type_add_abs:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f];
      break;
    case type_add_rel:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - target_curve[f]) * (min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]);
      break;
    case type_add_stk:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - modulated_curve[f]) * (min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]);
      break;
    case type_ab_abs:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += unipolar_to_bipolar(min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]) * 0.5f;
      break;
    case type_ab_rel:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - std::fabs(0.5f - target_curve[f]) * 2.0f) * unipolar_to_bipolar(min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]) * 0.5f;
      break;
    case type_ab_stk:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += (1 - std::fabs(0.5f - modulated_curve[f]) * 2.0f) * unipolar_to_bipolar(min_curve[f] + (max_curve[f] - min_curve[f]) * source_curve[f]) * 0.5f;
      break;
    default:
      assert(false);
      break;
    }
  }

  // clamp, modulated_curve_ptrs[r] will point to the first 
  // occurence of modulation, but mod effects are accumulated there
  for(int r = 0; r < route_count; r++)
    if(modulated_curve_ptrs[r] != nullptr)
      modulated_curve_ptrs[r]->transform(block.start_frame, block.end_frame, [](float v) { return std::clamp(v, 0.0f, 1.0f); });
  
  *block.state.own_context = &_mixdown;
}

}
