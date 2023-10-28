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
enum { 
  param_active, param_source, param_source_index, param_target, 
  param_target_index, param_target_param, param_target_param_index };

class cv_matrix_engine:
public module_engine { 
  cv_matrix_output _output = {};
  std::vector<module_topo const*> const _sources;
  std::vector<module_topo const*> const _targets;
  std::vector<std::vector<param_topo>> _modulatable_params;
public:
  void initialize() override {}
  void process(plugin_block& block) override;

  cv_matrix_engine(
    plugin_topo const& topo,
    std::vector<module_topo const*> const& sources,
    std::vector<module_topo const*> const& targets,
    std::vector<std::vector<param_topo>> const& modulatable_params);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_matrix_engine);
};

module_topo 
cv_matrix_topo(
  int section,
  plugin_base::module_gui_colors const& colors, 
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
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 2, 1, 2, 1, 2, 1 } })));
  
  result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", param_active, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(vector_map(sources, list_item::from_topo_ptr<module_topo>), ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });
  
  auto map_to_slot_domain_ptr = [](auto const& t) { return make_domain_step(0, t->info.slot_count - 1, 1, 1); };
  auto source_slot_domains = vector_map(sources, map_to_slot_domain_ptr);
  auto& source_index = result.params.emplace_back(make_param(
    make_topo_info("{5F6A54E9-50E6-4CDE-ACCB-4BA118F06780}", "Source Index", param_source_index, route_count),
    make_param_dsp_block(param_automate::none), make_domain_dependent(source_slot_domains),
    make_param_gui(section_main, gui_edit_type::dependent, param_layout::vertical, { 0, 2 }, make_label_none())));
  source_index.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });
  source_index.dependent.bind({ param_source }, vector_explicit_copy(source_slot_domains), [](int const* vs) { return vs[0]; });

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(vector_map(targets, list_item::from_topo_ptr<module_topo>), ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 3 }, make_label_none())));
  target.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });
  
  auto target_slot_domains = vector_map(targets, map_to_slot_domain_ptr);
  auto& target_index = result.params.emplace_back(make_param(
    make_topo_info("{79366858-994F-485F-BA1F-34AE3DFD2CEE}", "Target Index", param_target_index, route_count),
    make_param_dsp_block(param_automate::none), make_domain_dependent(target_slot_domains),
    make_param_gui(section_main, gui_edit_type::dependent, param_layout::vertical, { 0, 4 }, make_label_none())));
  target_index.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });
  target_index.dependent.bind({ param_target }, vector_explicit_copy(target_slot_domains), [](int const* vs) { return vs[0]; });

  std::vector<param_domain> target_param_domains;
  auto is_modulatable = [](auto const& p) { return p.dsp.automate == param_automate::modulate; };
  auto filter_modulatable = [is_modulatable](auto const& ps) { return vector_filter(ps, is_modulatable); };
  auto map_params_to_items = [](auto const& ps) { return vector_map(ps, list_item::from_topo<param_topo>); };
  auto map_items_to_domains = [](auto const& is) { return make_domain_item(is, ""); };
  auto target_params = vector_map(targets, [](auto const& m) { return m->params; });
  auto modulatable_target_params = vector_map(target_params, filter_modulatable);
  auto modulatable_target_items = vector_map(modulatable_target_params, map_params_to_items);
  auto modulatable_target_domains = vector_map(modulatable_target_items, map_items_to_domains);
  auto& target_param = result.params.emplace_back(make_param(
    make_topo_info("{EA395DC3-A357-4B76-BBC9-CE857FB9BC2A}", "Target Param", param_target_param, route_count),
    make_param_dsp_block(param_automate::none), make_domain_dependent(modulatable_target_domains),
    make_param_gui(section_main, gui_edit_type::dependent, param_layout::vertical, { 0, 5 }, make_label_none())));
  target_param.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });
  target_param.dependent.bind({ param_target }, vector_explicit_copy(modulatable_target_domains), [](int const* vs) { return vs[0]; });

  auto map_to_slot_domain = [](auto const& t) { return make_domain_step(0, t.info.slot_count - 1, 1, 1); };
  auto modulatable_target_params_joined = vector_join(modulatable_target_params);
  auto modulatable_target_param_index_domain_mappings = vector_index_count(modulatable_target_params);
  auto modulatable_target_param_index_domains = vector_map(modulatable_target_params_joined, map_to_slot_domain);
  auto& target_param_index = result.params.emplace_back(make_param(
    make_topo_info("{05E7FB15-58AD-40EA-BA7F-FDAB255879ED}", "Target Param Index", param_target_param_index, route_count),
    make_param_dsp_block(param_automate::none), make_domain_dependent(modulatable_target_param_index_domains),
    make_param_gui(section_main, gui_edit_type::dependent, param_layout::vertical, { 0, 6 }, make_label_none())));
  target_param_index.gui.bindings.enabled.bind({ param_active }, [](auto const& vs) { return vs[0] != 0; });
  target_param_index.dependent.bind({ param_target, param_target_param }, vector_explicit_copy(modulatable_target_param_index_domains), 
    [mappings = modulatable_target_param_index_domain_mappings](int const* vs) { return mappings[vs[0]][vs[1]]; });

  result.engine_factory = [sources, targets, modulatable_target_params](auto const& topo, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(topo, sources, targets, modulatable_target_params); };
  return result;
}

cv_matrix_engine::
cv_matrix_engine(
  plugin_topo const& topo,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets,
  std::vector<std::vector<param_topo>> const& modulatable_params) :
  _sources(sources), _targets(targets), _modulatable_params(modulatable_params)
{
  plugin_dims dims(topo);
  _output.modulation.resize(dims.module_slot_param_slot);
  _output.modulation_index.resize(dims.module_slot_param_slot);
}

void
cv_matrix_engine::process(plugin_block& block)
{
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
}

}
