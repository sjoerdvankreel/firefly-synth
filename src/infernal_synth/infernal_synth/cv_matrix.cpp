#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>

#include <infernal_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

static int constexpr groute_count = 6;
static int constexpr vroute_count = 12;

enum { section_main };
enum { type_off, type_mul, type_add, type_addbi };
enum { param_type, param_source, param_target, param_amount };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{7CE8B8A1-0711-4BDE-BDFF-0F97BF16EB57}", "Off");
  result.emplace_back("{C185C0A7-AE6A-4ADE-8171-119A96C24233}", "Mul");
  result.emplace_back("{000C0860-B191-4554-9249-85846B1AFFD1}", "Add");
  result.emplace_back("{169406D2-E86F-4275-A49F-59ED67CD7661}", "AddBi");
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
  void initialize() override {}
  void process(plugin_block& block) override;

  cv_matrix_engine(
    bool global,
    plugin_topo const& topo, 
    std::vector<module_output_mapping> const& sources,
    std::vector<param_topo_mapping> const& targets);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_matrix_engine);
};

static void
init_voice_default(plugin_state& state)
{
  state.set_text_at(module_vcv_matrix, 0, param_type, 0, "Add");
  state.set_text_at(module_vcv_matrix, 0, param_source, 0, "Env 2");
  state.set_text_at(module_vcv_matrix, 0, param_target, 0, "VFX 1 Freq");
  state.set_text_at(module_vcv_matrix, 0, param_type, 1, "AddBi");
  state.set_text_at(module_vcv_matrix, 0, param_amount, 1, "33");
  state.set_text_at(module_vcv_matrix, 0, param_source, 1, "GLFO 2");
  state.set_text_at(module_vcv_matrix, 0, param_target, 1, "Osc 1 Bal");
  state.set_text_at(module_vcv_matrix, 0, param_type, 2, "AddBi");
  state.set_text_at(module_vcv_matrix, 0, param_source, 2, "In PB");
  state.set_text_at(module_vcv_matrix, 0, param_target, 2, "Osc 1 PB");
  state.set_text_at(module_vcv_matrix, 0, param_type, 3, "AddBi");
  state.set_text_at(module_vcv_matrix, 0, param_source, 3, "In PB");
  state.set_text_at(module_vcv_matrix, 0, param_target, 3, "Osc 2 PB");
}

static void
init_global_default(plugin_state& state)
{
  state.set_text_at(module_gcv_matrix, 0, param_type, 0, "AddBi");
  state.set_text_at(module_gcv_matrix, 0, param_amount, 0, "33");
  state.set_text_at(module_gcv_matrix, 0, param_source, 0, "GLFO 1");
  state.set_text_at(module_gcv_matrix, 0, param_target, 0, "GFX 1 Freq");
  state.set_text_at(module_gcv_matrix, 0, param_type, 1, "Add");
  state.set_text_at(module_gcv_matrix, 0, param_source, 1, "In Mod");
  state.set_text_at(module_gcv_matrix, 0, param_target, 1, "GFX 1 Freq");
}

void
select_midi_active(
  plugin_state const& state, int slot, bool global,
  std::vector<module_output_mapping> const& mappings, jarray<int, 3>& active)
{
  int route_count = global? groute_count: vroute_count;
  int module = global? module_gcv_matrix: module_vcv_matrix;

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
          active[module_midi][0][midi_source_pb] = 1;
        else if (mapping.output_index == midi_output_cc)
          active[module_midi][0][midi_source_cc + mapping.output_slot] = 1;
        else
          assert(false);
      }
    }
  }
}

module_topo
cv_matrix_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos, bool global,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets)
{
  auto const voice_info = make_topo_info("{5F794E80-735C-43E8-B8EC-83910D118AF0}", "VCV", module_vcv_matrix, 1);
  auto const global_info = make_topo_info("{DB22D4C1-EDA5-45F6-AE9B-183CA6F4C28D}", "GCV", module_gcv_matrix, 1);
  int route_count = global ? groute_count : vroute_count;
  auto const info = topo_info(global? global_info: voice_info);
  module_stage stage = global ? module_stage::input : module_stage::voice;

  module_topo result(make_module(info,
    make_module_dsp(stage, module_output::cv, 0, {
      make_module_dsp_output(false, make_topo_info("{3AEE42C9-691E-484F-B913-55EB05CFBB02}", "Output", 0, route_count)) }),
    make_module_gui(section, colors, pos, { 1, 1 })));
  result.gui.tabbed_name = global ? "Global" : "Voice";

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, -30 } })));
  main.gui.scroll_mode = global? gui_scroll_mode::none: gui_scroll_mode::vertical;
  
  result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Type", param_type, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui(section_main, gui_edit_type::autofit_list, param_layout::vertical, { 0, 0 }, make_label_none())));

  auto source_matrix = make_output_matrix(sources);
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(source_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  source.gui.submenu = source_matrix.submenu;

  auto target_matrix = make_param_matrix(targets);
  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(target_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  target.gui.submenu = target_matrix.submenu;
  target.gui.item_enabled.auto_bind = true;

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{95153B11-6CA7-42EE-8709-9C3359CF23C8}", "Amount", param_amount, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  amount.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  result.default_initializer = global ? init_global_default : init_voice_default;
  result.midi_active_selector = [global, sm = source_matrix.mappings](
    plugin_state const& state, int slot, jarray<int, 3>& active) { select_midi_active(state, slot, global, sm, active); };
  result.engine_factory = [global, sm = source_matrix.mappings, tm = target_matrix.mappings]
    (auto const& topo, int, int) -> std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(global, topo, sm, tm); };
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
  plugin_dims dims(topo);
  _mixdown.resize(dims.module_slot_param_slot);
  _modulation_indices.resize(dims.module_slot_param_slot);
}

void
cv_matrix_engine::process(plugin_block& block)
{
  int route_count = _global? groute_count: vroute_count;

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
  jarray<float, 1>* vmodulated_curve_ptrs[vroute_count] = { nullptr };
  jarray<float, 1>* gmodulated_curve_ptrs[groute_count] = { nullptr };
  jarray<float, 1>** modulated_curve_ptrs = _global? gmodulated_curve_ptrs: vmodulated_curve_ptrs;
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
    auto const& amount_curve = block.state.own_accurate_automation[param_amount][r];
    switch (type)
    {
    case type_add:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += source_curve[f] * amount_curve[f];
      break;
    case type_addbi:
      for (int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] += unipolar_to_bipolar(source_curve[f]) * amount_curve[f] * 0.5f;
      break;
    case type_mul:
      for(int f = block.start_frame; f < block.end_frame; f++)
        modulated_curve[f] = mix_signal(amount_curve[f], modulated_curve[f], source_curve[f] * modulated_curve[f]);
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
