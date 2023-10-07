#include <plugin_base/support.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { param_sync, param_rate, param_num, param_denom };

class lfo_engine: 
public module_engine {
  float _phase;

public:
  lfo_engine() { initialize(); }
  INF_DECLARE_MOVE_ONLY(lfo_engine);
  void process(plugin_block& block) override;
  void initialize() override { _phase = 0; }
};

module_topo
lfo_topo()
{
  module_topo result(make_module(
    "{FAF92753-C6E4-4D78-BD7C-584EF473E29F}", "Global LFO", module_lfo, 3, 
    module_stage::input, module_output::cv, 1,
    gui_layout::tabbed, gui_position { 0, 0 }, gui_dimension { 1, 1 }));
  result.sections.emplace_back(make_section(
    "{F0002F24-0CA7-4DF3-A5E3-5B33055FD6DC}", "Main", section_main, gui_position{ 0, 0 }, gui_dimension{ 1, 4 }));
  result.engine_factory = [](int, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<lfo_engine>(); };

  result.params.emplace_back(param_toggle(
    "{2A9CAE77-13B0-406F-BA57-1A30ED2F5D80}", "Sync", param_sync, 1, section_main, false,
    param_direction::input,
    gui_label_contents::name, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position{ 0, 0 }));

  result.params.emplace_back(param_linear(
    "{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", param_rate, 1, section_main, 0.1, 20, 1, 2, "Hz",
    param_direction::input, param_rate::accurate, param_format::plain, gui_edit_type::knob,
    gui_label_contents::both, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));

  result.params.emplace_back(param_steps(
    "{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Num", param_num, 1, section_main, 1, 16, 1,
    param_direction::input, gui_edit_type::list,
    gui_label_contents::name, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position{ 0, 2 }));

  result.params.emplace_back(param_steps(
    "{84B58AC9-C401-4580-978C-60591AFB757B}", "Denom", param_denom, 1, section_main, 1, 16, 4,
    param_direction::input, gui_edit_type::list,
    gui_label_contents::name, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position{ 0, 3 }));

  return result;
}

void
lfo_engine::process(plugin_block& block)
{
}

}
