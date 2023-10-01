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
    "{1762278E-5B1E-4495-B499-060EE997A8FD}", "Voice CV Matrix", 1, 
    module_stage::voice, module_output::cv, 1,
    gui_layout::single, gui_position { 4, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(); };

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { 1, 1 }));
  result.params.emplace_back(param_toggle(
    "{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", route_count, section_main, false,
    param_dir::input,
    param_label_contents::none, param_label_align::left, param_label_justify::center,
    gui_layout::vertical, gui_position { 0, 0 }));

  return result;
}

void
cv_matrix_engine::process(process_block& block)
{
}

}
