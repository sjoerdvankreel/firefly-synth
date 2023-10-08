#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

static int constexpr route_count = 8;

enum { section_main };
enum { param_on, param_active, param_dynamic_begin };

static std::vector<list_item>
module_items(std::vector<module_topo const*> const& modules)
{
  std::vector<list_item> result;
  for(int i = 0; i < modules.size(); i++)
    result.push_back(list_item((*modules[i]).info));
  return result;
}

class cv_matrix_engine:
public module_engine { 
public:
  void initialize() override {}
  void process(plugin_block& block) override;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(cv_matrix_engine);
};

module_topo 
cv_matrix_topo(
  std::vector<module_topo const*> const& sources, 
  std::vector<module_topo const*> const& targets)
{
  std::vector<int> enabled_params = { param_on, param_active };
  gui_binding_selector enabled_selector = [](auto const& vs, auto const&) { return vs[0] != 0 && vs[1] != 0; };

  module_topo result(make_module(
    make_topo_info("{1762278E-5B1E-4495-B499-060EE997A8FD}", "Voice CV Matrix", module_cv_matrix, 1), 
    make_module_dsp(module_stage::voice, module_output::cv, 1),
    make_module_gui(gui_layout::single, { 4, 0 }, { 1, 1 })));
  result.engine_factory = [](int, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(); };
  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main"), 
    make_section_gui({ 0, 0 }, { { 1, 5 }, { 1, 1, 1, 1, 1 } })));
  
  result.params.emplace_back(make_param(
    make_topo_info("{06512F9B-2B49-4C2E-BF1F-40070065CABB}", "On", param_on, 1),
    make_param_dsp_block(), make_domain_toggle(true),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0, 1, 5 }, 
      make_label_default(gui_label_contents::name))));  
  auto& active = result.params.emplace_back(make_param(
    make_topo_info("{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", param_active, route_count),
    make_param_dsp_block(), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, gui_layout::vertical, { 1, 0 }, 
      make_label_none())));
  active.gui.bindings.enabled.params = { param_on };
  active.gui.bindings.enabled.selector = [](auto const& vs, auto const&) { return vs[0] != 0; };
  
  int param_current = param_dynamic_begin;
  int param_source = param_current;
  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_current, route_count),
    make_param_dsp_block(), make_domain_item(module_items(sources), ""),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 1 }, 
      make_label_none())));
  source.gui.bindings.enabled.params = enabled_params;
  source.gui.bindings.enabled.selector = enabled_selector;
  param_current++;

  for(int i = 0; i < sources.size(); i++)
  {
    auto& source_index = result.params.emplace_back(make_param(
      make_topo_info({ "{5F6A54E9-50E6-4CDE-ACCB-4BA118F06780}", sources[i]->info.tag.id }, "Source Index", param_current, route_count),
      make_param_dsp_block(), make_domain_step(0, sources[i]->info.slot_count - 1, 0),
      make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 2 }, 
        make_label_none())));
    source_index.gui.bindings.enabled.params = enabled_params;
    source_index.gui.bindings.enabled.selector = enabled_selector;
    source_index.gui.bindings.visible.context = { i };
    source_index.gui.bindings.visible.params = { param_source };
    source_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };
    param_current++;
  }
  
  int param_target = param_current;
  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_current, route_count),
    make_param_dsp_block(), make_domain_item(module_items(targets), ""),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 3 }, 
      make_label_none())));
  target.gui.bindings.enabled.params = enabled_params;
  target.gui.bindings.enabled.selector = enabled_selector;
  param_current++;
  
  for(int i = 0; i < targets.size(); i++)
  {
    auto& target_index = result.params.emplace_back(make_param(
      make_topo_info({ "{79366858-994F-485F-BA1F-34AE3DFD2CEE}", targets[i]->info.tag.id }, "Target Index", param_current, route_count),
      make_param_dsp_block(), make_domain_step(0, targets[i]->info.slot_count - 1, 0),
      make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 4 }, 
        make_label_none())));
    target_index.gui.bindings.enabled.params = enabled_params;
    target_index.gui.bindings.enabled.selector = enabled_selector;
    target_index.gui.bindings.visible.context = { i };
    target_index.gui.bindings.visible.params = { param_target };
    target_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };
    param_current++;
  }

  return result;

#if 0
 
  auto& osc_target = result.params.emplace_back(make_param(
    make_topo_info("{28286D1C-6A9D-4CD4-AB70-4A3AFDF7302B}", "Osc Param", param++, route_count),
    make_param_dsp_block(), make_domain_item(target_params(osc), ""),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 5 }, 
      make_label_none())));
    osc_target.gui.bindings.enabled.params = enabled_params;
  osc_target.gui.bindings.enabled.selector = enabled_selector;
  osc_target.gui.bindings.visible.params = { param_target };
  osc_target.gui.bindings.visible.context = { index_of_item_tag(result.params[param_target].domain.items, module_osc) };
  osc_target.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };
  
  auto& filter_target = result.params.emplace_back(make_param(
    make_topo_info("{B8098815-BBD5-4171-9AAF-CE4B6645AEE2}", "Filter Param", param++, route_count),
    make_param_dsp_block(), make_domain_item(target_params(filter), ""),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 5 }, 
      make_label_none())));
  filter_target.gui.bindings.enabled.params = enabled_params;
  filter_target.gui.bindings.enabled.selector = enabled_selector;
  filter_target.gui.bindings.visible.params = { param_target };
  filter_target.gui.bindings.visible.context = { index_of_item_tag(result.params[param_target].domain.items, module_filter) };
  filter_target.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };
  
  int FILTER_PARAM_OSC_GAIN = 2; // TODO
  auto& osc_gain_index = result.params.emplace_back(make_param(
    make_topo_info("{FB4EB870-48DD-40D5-9D0E-2E9F0C4E3C48}", "Filter Osc Gain", param++, route_count),
    make_param_dsp_block(), make_domain_step(0, filter.params[FILTER_PARAM_OSC_GAIN].info.slot_count - 1, 0),
    make_param_gui(section_main, gui_edit_type::list, gui_layout::vertical, { 1, 6 }, 
      make_label_none())));
  osc_gain_index.gui.bindings.enabled.params = enabled_params;
  osc_gain_index.gui.bindings.enabled.selector = enabled_selector;
  osc_gain_index.gui.bindings.visible.params = { param_target, param_target_filter_param };
  osc_gain_index.gui.bindings.visible.context = {
    index_of_item_tag(result.params[param_target].domain.items, module_filter),
    index_of_item_tag(result.params[param_target_filter_param].domain.items, FILTER_PARAM_OSC_GAIN )};
  osc_gain_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0] && vs[1] == ctx[1]; };
  return result;

#endif
}

void
cv_matrix_engine::process(plugin_block& block)
{
}

}
