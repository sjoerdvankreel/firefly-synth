#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
static int constexpr route_count = 8;

static std::vector<item_topo>
source_items(module_topo const& lfo_topo, module_topo const& env_topo)
{
  std::vector<item_topo> result;
  result.emplace_back(lfo_topo);
  result.emplace_back(env_topo);
  return result;
}

static std::vector<item_topo>
target_items(module_topo const& osc_topo, module_topo const& filter_topo)
{
  std::vector<item_topo> result;
  result.emplace_back(osc_topo);
  result.emplace_back(filter_topo);
  return result;
}

static std::vector<item_topo>
target_osc_items(module_topo const& osc_topo)
{
  std::vector<item_topo> result;
  result.emplace_back(osc_topo.params[osc_param_bal]);
  result.emplace_back(osc_topo.params[osc_param_gain]);
  result.emplace_back(osc_topo.params[osc_param_cent]);
  return result;
}

static std::vector<item_topo>
target_filter_items(module_topo const& filter_topo)
{
  std::vector<item_topo> result;
  result.emplace_back(filter_topo.params[filter_param_freq]);
  result.emplace_back(filter_topo.params[filter_param_temp]);
  // TODO osc gain counted parameter
  return result;
}

class cv_matrix_engine:
public module_engine { 
public:
  void initialize() override {}
  void process(process_block& block) override;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(cv_matrix_engine);
};

module_topo 
cv_matrix_topo(
  module_topo const& lfo_topo,
  module_topo const& env_topo,
  module_topo const& osc_topo,
  module_topo const& filter_topo)
{
  module_topo result(make_module(
    "{1762278E-5B1E-4495-B499-060EE997A8FD}", "Voice CV Matrix", module_cv_matrix, 1, 
    module_stage::voice, module_output::cv, 1,
    gui_layout::single, gui_position { 4, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(); };
  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { { 1, 5 }, { 1, 1, 1, 1, 1, 1 } }));
  
  std::vector<int> enabled_params = { cv_matrix_param_on, cv_matrix_param_active };
  ui_state_selector enabled_selector = [](auto const& vs, auto const&) { return vs[0] != 0 && vs[1] != 0; };

  result.params.emplace_back(param_toggle(
    "{06512F9B-2B49-4C2E-BF1F-40070065CABB}", "On", cv_matrix_param_on, 1, section_main, true,
    param_dir::input,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 0, 1, 6 }));

  auto& active = result.params.emplace_back(param_toggle(
    "{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", cv_matrix_param_active, route_count, section_main, false,
    param_dir::input,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position { 1, 0 }));
  active.ui_state.enabled_params = { cv_matrix_param_on };
  active.ui_state.enabled_selector = [](auto const& vs, auto const&) { return vs[0] != 0; };

  auto& source = result.params.emplace_back(param_items(
    "{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", cv_matrix_param_source, route_count, section_main, source_items(lfo_topo, env_topo), "",
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 1 }));
  source.ui_state.enabled_params = enabled_params;
  source.ui_state.enabled_selector = enabled_selector;

  auto& lfo_index = result.params.emplace_back(param_steps(
    "{5F6A54E9-50E6-4CDE-ACCB-4BA118F06780}", "LFO Index", cv_matrix_param_lfo_index, route_count, section_main, 0, lfo_topo.slot_count - 1, 0,
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 2 }));
  lfo_index.ui_state.enabled_params = enabled_params;
  lfo_index.ui_state.enabled_selector = enabled_selector;
  lfo_index.ui_state.visibility_params = { cv_matrix_param_source };
  lfo_index.ui_state.visibility_context = { index_of_item_tag(result.params[cv_matrix_param_source].items, module_lfo)};
  lfo_index.ui_state.visibility_selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& env_index = result.params.emplace_back(param_steps(
    "{BA2FB14A-5484-4721-B640-DA26306194A4}", "Env Index", cv_matrix_param_env_index, route_count, section_main, 0, env_topo.slot_count - 1, 0,
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 2 }));
  env_index.ui_state.enabled_params = enabled_params;
  env_index.ui_state.enabled_selector = enabled_selector;
  env_index.ui_state.visibility_params = { cv_matrix_param_source };
  env_index.ui_state.visibility_context = { index_of_item_tag(result.params[cv_matrix_param_source].items, module_env) };
  env_index.ui_state.visibility_selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& target = result.params.emplace_back(param_items(
    "{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", cv_matrix_param_target, route_count, section_main, target_items(osc_topo, filter_topo), "",
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 3 }));
  target.ui_state.enabled_params = enabled_params;
  target.ui_state.enabled_selector = enabled_selector;

  auto& osc_index = result.params.emplace_back(param_steps(
    "{79366858-994F-485F-BA1F-34AE3DFD2CEE}", "Osc Index", cv_matrix_param_osc_index, route_count, section_main, 0, osc_topo.slot_count - 1, 0,
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 4 }));
  osc_index.ui_state.enabled_params = enabled_params;
  osc_index.ui_state.enabled_selector = enabled_selector;
  osc_index.ui_state.visibility_params = { cv_matrix_param_target };
  osc_index.ui_state.visibility_context = { index_of_item_tag(result.params[cv_matrix_param_target].items, module_osc) };
  osc_index.ui_state.visibility_selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& osc_target = result.params.emplace_back(param_items(
    "{28286D1C-6A9D-4CD4-AB70-4A3AFDF7302B}", "Osc Target", cv_matrix_param_osc_target, route_count, section_main, target_osc_items(osc_topo), "",
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 5 }));
  osc_target.ui_state.enabled_params = enabled_params;
  osc_target.ui_state.enabled_selector = enabled_selector;
  osc_target.ui_state.visibility_params = { cv_matrix_param_osc_target };
  osc_target.ui_state.visibility_context = { index_of_item_tag(result.params[cv_matrix_param_target].items, module_osc) };
  osc_target.ui_state.visibility_selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& filter_target = result.params.emplace_back(param_items(
    "{B8098815-BBD5-4171-9AAF-CE4B6645AEE2}", "Filter Target", cv_matrix_param_filter_target, route_count, section_main, target_filter_items(filter_topo), "",
    param_dir::input, param_edit::list,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 5 }));
  filter_target.ui_state.enabled_params = enabled_params;
  filter_target.ui_state.enabled_selector = enabled_selector;
  filter_target.ui_state.visibility_params = { cv_matrix_param_filter_target };
  filter_target.ui_state.visibility_context = { index_of_item_tag(result.params[cv_matrix_param_target].items, module_filter) };
  filter_target.ui_state.visibility_selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  return result;
}

void
cv_matrix_engine::process(process_block& block)
{
}

}
