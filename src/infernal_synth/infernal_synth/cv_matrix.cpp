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

struct source_mapping
{
  int topo;
  int slot;
};

struct target_mapping
{
  int module_topo;
  int module_slot;
  int param_topo;
  int param_slot;
};

static int constexpr route_count = 8;

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
  cv_matrix_output _output = {};
  jarray<int, 4> _modulation_indices = {};
  std::vector<source_mapping> const _sources;
  std::vector<target_mapping> const _targets;
public:
  void initialize() override {}
  void process(plugin_block& block) override;

  cv_matrix_engine(
    plugin_topo const& topo, 
    std::vector<source_mapping> const& sources, 
    std::vector<target_mapping> const& targets);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_matrix_engine);
};

module_topo 
cv_matrix_topo(
  int section,
  plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets)
{
  module_topo result(make_module(
    make_topo_info("{1762278E-5B1E-4495-B499-060EE997A8FD}", "CV", module_cv_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::cv, route_count, 0),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, -30 } })));
  
  result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Type", param_type, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui(section_main, gui_edit_type::autofit_list, param_layout::vertical, { 0, 0 }, make_label_none())));

  int source_index = 0;
  std::vector<list_item> source_items;
  std::vector<source_mapping> source_mappings;
  auto source_submenu = std::make_shared<gui_submenu>();
  for(int m = 0; m < sources.size(); m++)
  {
    auto const& tag = sources[m]->info.tag;
    auto module_submenu = std::make_shared<gui_submenu>();
    module_submenu->name = tag.name;
    for(int mi = 0; mi < sources[m]->info.slot_count; mi++)
    {
      list_item item;
      item.id = tag.id + "-" + std::to_string(mi);
      item.name = tag.name + " " + std::to_string(mi + 1);
      source_items.push_back(item);
      module_submenu->indices.push_back(source_index++);
      source_mappings.push_back({ sources[m]->info.index, mi });
    }
    source_submenu->children.push_back(module_submenu);
  }
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(source_items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  source.gui.submenu = source_submenu;

  int target_index = 0;
  std::vector<list_item> target_items;
  std::vector<target_mapping> target_mappings;
  auto target_submenu = std::make_shared<gui_submenu>();
  for(int m = 0; m < targets.size(); m++)
  {
    auto const& module_tag = targets[m]->info.tag;
    auto module_submenu = std::make_shared<gui_submenu>();
    module_submenu->name = module_tag.name;
    for (int mi = 0; mi < targets[m]->info.slot_count; mi++)
    {
      auto module_slot_submenu = std::make_shared<gui_submenu>();
      module_slot_submenu->name = module_tag.name + " " + std::to_string(mi + 1);
      for(int p = 0; p < targets[m]->params.size(); p++)
        if(targets[m]->params[p].dsp.can_modulate(mi))
        {
          auto const& param_tag = targets[m]->params[p].info.tag;
          for (int pi = 0; pi < targets[m]->params[p].info.slot_count; pi++)
          {
            list_item item;
            item.id = module_tag.id + "-" + std::to_string(mi) + "-" + param_tag.id + "-" + std::to_string(pi);
            item.name = module_tag.name + " " + std::to_string(mi + 1) + " " + param_tag.name;
            if(targets[m]->params[p].info.slot_count > 1) item.name += " " + std::to_string(pi + 1);
            target_items.push_back(item);
            module_slot_submenu->indices.push_back(target_index++);
            target_mappings.push_back({ targets[m]->info.index, mi, targets[m]->params[p].info.index, pi });
          }
        }
      module_submenu->children.push_back(module_slot_submenu);
    }
    target_submenu->children.push_back(module_submenu);
  }
  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(target_items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });
  target.gui.submenu = target_submenu;

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{95153B11-6CA7-42EE-8709-9C3359CF23C8}", "Amount", param_amount, route_count),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  amount.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_off; });

  result.engine_factory = [source_mappings, target_mappings](auto const& topo, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(topo, source_mappings, target_mappings); };
  return result;
}

cv_matrix_engine::
cv_matrix_engine(
  plugin_topo const& topo,
  std::vector<source_mapping> const& sources,
  std::vector<target_mapping> const& targets):
_sources(sources), _targets(targets)
{
  plugin_dims dims(topo);
  _output.resize(dims.module_slot_param_slot);
  _modulation_indices.resize(dims.module_slot_param_slot);
}

void
cv_matrix_engine::process(plugin_block& block)
{
  // set every modulatable parameter to its corresponding automation curve
  for (int m = 0; m < _targets.size(); m++)
  {
    int tp = _targets[m].param_topo;
    int tpi = _targets[m].param_slot;
    int tm = _targets[m].module_topo;
    int tmi = _targets[m].module_slot;
    _modulation_indices[tm][tmi][tp][tpi] = -1;
    auto const& curve = block.state.all_accurate_automation[tm][tmi][tp][tpi];
    _output[tm][tmi][tp][tpi] = &curve;
  }

  // apply modulation routing
  int modulation_index = 0;
  auto const& own_automation = block.state.own_block_automation;
  jarray<float, 1>* modulated_curve_ptrs[route_count] = { nullptr };
  for (int r = 0; r < route_count; r++)
  {
    jarray<float, 1>* modulated_curve_ptr = nullptr;
    int type = own_automation[param_type][r].step();
    if(type == type_off) continue;

    // found out indices of modulation target
    int selected_target = own_automation[param_target][r].step();
    int tp = _targets[selected_target].param_topo;
    int tpi = _targets[selected_target].param_slot;
    int tm = _targets[selected_target].module_topo;
    int tmi = _targets[selected_target].module_slot;

    // if already modulated, set target curve to own buffer
    int existing_modulation_index = _modulation_indices[tm][tmi][tp][tpi];
    if (existing_modulation_index != -1)
      modulated_curve_ptr = &block.state.own_cv[existing_modulation_index];
    else
    {
      // else pick the next of our own cv outputs
      modulated_curve_ptr = &block.state.own_cv[modulation_index];
      auto const& target_automation = block.state.all_accurate_automation[tm][tmi][tp][tpi];
      target_automation.copy_to(block.start_frame, block.end_frame, *modulated_curve_ptr);
      modulated_curve_ptrs[r] = modulated_curve_ptr;
      _output[tm][tmi][tp][tpi] = modulated_curve_ptr;
      _modulation_indices[tm][tmi][tp][tpi] = modulation_index++;
    }

    assert(modulated_curve_ptr != nullptr);
    jarray<float, 1>& modulated_curve = *modulated_curve_ptr;

    // find out indices of modulation source
    int selected_source = own_automation[param_source][r].step();
    int sm = _sources[selected_source].topo;
    int smi = _sources[selected_source].slot;
    auto const& source_curve = block.plugin.modules[sm].dsp.stage == module_stage::voice
      ? block.voice->all_cv[sm][smi][0]
      : block.state.all_global_cv[sm][smi][0];

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
  
  *block.state.own_context = &_output;
}

}
