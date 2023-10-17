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
enum { param_on, param_active, param_source, param_source_index, param_target, param_target_index, param_target_param };

class cv_matrix_engine:
public module_engine { 
  cv_matrix_output _output = {};
  std::vector<module_topo const*> const _sources;
  std::vector<module_topo const*> const _targets;
public:
  void initialize() override {}
  void process(plugin_block& block) override;

  cv_matrix_engine(
    plugin_topo const& topo,
    std::vector<module_topo const*> const& sources,
    std::vector<module_topo const*> const& targets);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_matrix_engine);
};

module_topo 
cv_matrix_topo(
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets)
{
  std::vector<int> enabled_params = { param_on, param_active };
  gui_binding_selector enabled_selector = [](auto const& vs) { return vs[0] != 0 && vs[1] != 0; };

  module_topo result(make_module(
    make_topo_info("{1762278E-5B1E-4495-B499-060EE997A8FD}", "Voice CV Matrix", module_cv_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::cv, route_count),
    make_module_gui(gui_layout::single, { 2, 0 }, { 1, 1 })));
  result.engine_factory = [sources, targets](auto const& topo, int, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(topo, sources, targets); };
  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main"), 
    make_section_gui({ 0, 0 }, { { 1, 5 }, { 1, 1, 1, 1, 1, 1 } })));
  
  result.params.emplace_back(make_param(
    make_topo_info("{06512F9B-2B49-4C2E-BF1F-40070065CABB}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(true),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0, 1, 6 }, 
      make_label_default(gui_label_contents::name))));
  
  auto& active = result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", param_active, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, gui_layout::vertical, { 1, 0 }, 
      make_label_none())));
  active.gui.bindings.enabled.params = { param_on };
  active.gui.bindings.enabled.selector = [](auto const& vs) { return vs[0] != 0; };
  
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(vector_map(sources, list_item::from_topo_ptr<module_topo>), ""),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 1 }, 
      make_label_none())));
  source.gui.bindings.enabled.params = enabled_params;
  source.gui.bindings.enabled.selector = enabled_selector;
  
  auto map_to_slot_domain = [](auto const& m) { return make_domain_step(0, m->info.slot_count - 1, 1, 1); };
  auto source_slot_domains = vector_map(sources, map_to_slot_domain);
  auto& source_index = result.params.emplace_back(make_param(
    make_topo_info("{5F6A54E9-50E6-4CDE-ACCB-4BA118F06780}", "Source Index", param_source_index, route_count),
    make_param_dsp_block(param_automate::none), make_domain_dependent(source_slot_domains),
    make_param_gui(section_main, gui_edit_type::dependent, gui_layout::vertical, { 1, 2 }, 
      make_label_none())));
  source_index.gui.bindings.enabled.params = enabled_params;
  source_index.gui.bindings.enabled.selector = enabled_selector;
  source_index.dependency_index = param_source;
  source_index.dependent_domains = vector_explicit_copy(source_slot_domains);

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(vector_map(targets, list_item::from_topo_ptr<module_topo>), ""),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 3 }, 
      make_label_none())));
  target.gui.bindings.enabled.params = enabled_params;
  target.gui.bindings.enabled.selector = enabled_selector;
  
  auto target_slot_domains = vector_map(targets, map_to_slot_domain);
  auto& target_index = result.params.emplace_back(make_param(
    make_topo_info("{79366858-994F-485F-BA1F-34AE3DFD2CEE}", "Target Index", param_target_index, route_count),
    make_param_dsp_block(param_automate::none), make_domain_dependent(target_slot_domains),
    make_param_gui(section_main, gui_edit_type::dependent, gui_layout::vertical, { 1, 4 },
      make_label_none())));
  target_index.gui.bindings.enabled.params = enabled_params;
  target_index.gui.bindings.enabled.selector = enabled_selector;
  target_index.dependency_index = param_target;
  target_index.dependent_domains = vector_explicit_copy(target_slot_domains);

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
    make_param_gui(section_main, gui_edit_type::dependent, gui_layout::vertical, { 1, 5 },
      make_label_default(gui_label_contents::value))));
  target_param.gui.bindings.enabled.params = enabled_params;
  target_param.gui.bindings.enabled.selector = enabled_selector;
  target_param.dependency_index = param_target;
  target_param.dependent_domains = vector_explicit_copy(modulatable_target_domains);

  return result;
}

cv_matrix_engine::
cv_matrix_engine(
  plugin_topo const& topo,
  std::vector<module_topo const*> const& sources,
  std::vector<module_topo const*> const& targets) :
  _sources(sources), _targets(targets)
{
  plugin_dims dims(topo);
  _output.modulation.resize(dims.module_slot_param_slot);
}

void
cv_matrix_engine::process(plugin_block& block)
{
  for(int m = 0; m < _targets.size(); m++)
    for(int p = 0; p < _targets[m]->params.size(); p++)
      if(_targets[m]->params[p].dsp.automate == param_automate::modulate)
        for (int mi = 0; mi < _targets[m]->info.slot_count; mi++)
          for(int pi = 0; pi < _targets[m]->params[p].info.slot_count; pi++)
            _output.modulation[_targets[m]->info.index][mi][p][pi] = 
              &block.state.all_accurate_automation[_targets[m]->info.index][mi][p][pi];
  *block.state.own_context = &_output;
}

}
