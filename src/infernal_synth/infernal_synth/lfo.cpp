#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/engine.hpp>
#include <plugin_base/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };

class lfo_engine: 
public module_engine {
  float _phase;

public:
  lfo_engine() { initialize(); }
  INF_DECLARE_MOVE_ONLY(lfo_engine);
  void process(process_block& block) override;
  void initialize() override { _phase = 0; }
};

module_topo
lfo_topo()
{
  module_topo result(make_module(
    "{FAF92753-C6E4-4D78-BD7C-584EF473E29F}", "Global LFO", 3, 
    module_stage::input, module_output::cv, 
    gui_layout::tabbed, gui_position { 0, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<lfo_engine>(); };

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { 1, 4 }));
  result.params.emplace_back(param_toggle(
    "{2A9CAE77-13B0-406F-BA57-1A30ED2F5D80}", "Sync", 1, section_main, false,
    param_dir::input,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position{ 0, 0 }));
  result.params.emplace_back(param_linear(
    "{EE68B03D-62F0-4457-9918-E3086B4BCA1C}", "Rate", 1, section_main, 0.1, 20, 1, 2, "Hz",
    param_dir::input, param_rate::accurate, param_format::plain, param_edit::knob,
    param_label_contents::both, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));
  result.params.emplace_back(param_steps(
    "{5D05DF07-9B42-46BA-A36F-E32F2ADA75E0}", "Num", 1, section_main, 1, 16, 1,
    param_dir::input, param_edit::list,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position{ 0, 2 }));
  result.params.emplace_back(param_steps(
    "{84B58AC9-C401-4580-978C-60591AFB757B}", "Denom", 1, section_main, 1, 16, 4,
    param_dir::input, param_edit::list,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position{ 0, 3 }));

  return result;
}

void
lfo_engine::process(process_block& block)
{
}

}
