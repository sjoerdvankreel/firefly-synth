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

static int constexpr route_count = 8;

enum { section_main };
enum { param_active, param_source, param_target };

class cv_matrix_engine:
public module_engine { 
  cv_matrix_output _output = {};
public:
  void initialize() override {}
  void process(plugin_block& block) override;

  cv_matrix_engine(plugin_topo const& topo);
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
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1 } })));
  
  result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", param_active, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  
  std::vector<list_item> source_items;
  for(int m = 0; m < sources.size(); m++)
    for(int mi = 0; mi < sources[m]->info.slot_count; mi++)
    {
      list_item item;
      auto const& tag = sources[m]->info.tag;
      item.id = tag.id + "-" + std::to_string(mi);
      item.name = tag.name + " " + std::to_string(mi + 1);
      source_items.push_back(item);
    }
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(source_items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });

  std::vector<list_item> target_items;
  for(int m = 0; m < targets.size(); m++)
    for (int mi = 0; mi < targets[m]->info.slot_count; mi++)
      for(int p = 0; p < targets[m]->params.size(); p++)
        if(targets[m]->params[p].dsp.automate == param_automate::modulate)
          for (int pi = 0; pi < targets[m]->params[p].info.slot_count; pi++)
          {
            list_item item;
            auto const& module_tag = targets[m]->info.tag;
            auto const& param_tag = targets[m]->params[p].info.tag;
            item.id = module_tag.id + "-" + std::to_string(mi) + "-" + param_tag.id + "-" + std::to_string(pi);
            item.name = module_tag.name + " " + std::to_string(mi + 1) + " " + param_tag.name;
            if(targets[m]->params[p].info.slot_count > 1) item.name += " " + std::to_string(pi + 1);
            target_items.push_back(item);
          }

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(target_items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });

  result.engine_factory = [](auto const& topo, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(topo); };
  return result;
}

cv_matrix_engine::
cv_matrix_engine(plugin_topo const& topo)
{
  plugin_dims dims(topo);
  _output.modulation.resize(dims.module_slot_param_slot);
  _output.modulation_index.resize(dims.module_slot_param_slot);
}

void
cv_matrix_engine::process(plugin_block& block)
{
#if 0
  // set every modulatable parameter to its corresponding automation curve
  for (int tm = 0; tm < _targets.size(); tm++)
    for (int tmi = 0; tmi < _targets[tm]->info.slot_count; tmi++)
      for (int tp = 0; tp < _modulatable_params[tm].size(); tp++)
        for (int tpi = 0; tpi < _modulatable_params[tm][tp].info.slot_count; tpi++)
        {
          int real_tm = _targets[tm]->info.index;
          int real_tp = _modulatable_params[tm][tp].info.index;
          _output.modulation_index[real_tm][tmi][real_tp][tpi] = -1;
          auto const& curve = block.state.all_accurate_automation[real_tm][tmi][real_tp][tpi];
          _output.modulation[real_tm][tmi][real_tp][tpi] = &curve;
        }

  // apply modulation routing
  int modulation_index = 0;
  auto const& own_automation = block.state.own_block_automation;
  for (int r = 0; r < route_count; r++)
  {
    jarray<float, 1>* modulated_curve_ptr = nullptr;
    if(own_automation[param_active][r].step() == 0) continue;

    // found out indices of modulation target
    int selected_target = own_automation[param_target][r].step();
    int tm = _targets[selected_target]->info.index;
    int tmi = own_automation[param_target_index][r].step();
    int tpi = own_automation[param_target_param_index][r].step();
    int tp = _modulatable_params[selected_target][own_automation[param_target_param][r].step()].info.index;

    // if already modulated, set target curve to own buffer
    int existing_modulation_index = _output.modulation_index[tm][tmi][tp][tpi];
    if (existing_modulation_index != -1)
      modulated_curve_ptr = &block.state.own_cv[existing_modulation_index];
    else
    {
      // else pick the next of our own cv outputs
      modulated_curve_ptr = &block.state.own_cv[modulation_index];
      auto const& target_automation = block.state.all_accurate_automation[tm][tmi][tp][tpi];
      std::copy(target_automation.cbegin(), target_automation.cend(), modulated_curve_ptr->begin());
      _output.modulation[tm][tmi][tp][tpi] = modulated_curve_ptr;
      _output.modulation_index[tm][tmi][tp][tpi] = modulation_index++;
    }

    assert(modulated_curve_ptr != nullptr);
    jarray<float, 1>& modulated_curve = *modulated_curve_ptr;

    // find out indices of modulation source
    int selected_source = own_automation[param_source][r].step();
    int source_module = _sources[selected_source]->info.index;
    int source_module_index = own_automation[param_source_index][r].step();
    auto const& source_curve = _sources[selected_source]->dsp.stage == module_stage::voice
      ? block.voice->all_cv[source_module][source_module_index][0]
      : block.state.all_global_cv[source_module][source_module_index][0];

    // apply modulation
    for(int f = block.start_frame; f < block.end_frame; f++)
    {
      modulated_curve[f] *= source_curve[f];
      check_unipolar(modulated_curve[f]);
    }
  }
  
  *block.state.own_context = &_output;
#endif
}

}
