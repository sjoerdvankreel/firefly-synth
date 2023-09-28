#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/engine.hpp>
#include <plugin_base/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

class env_engine: 
public module_engine {
  int _position;
public:
  env_engine() { initialize(); }
  INF_DECLARE_MOVE_ONLY(env_engine);
  void process(process_block& block) override;
  void initialize() override { _position = 0; }
};

enum { section_main };
enum { param_a, param_d, param_s, param_r };

module_topo
env_topo()
{
  module_topo result(make_module(
    "{DE952BFA-88AC-4F05-B60A-2CEAF9EE8BF9}", "Env", 1, 
    module_stage::voice, module_output::cv, 
    gui_layout::single, gui_position { 1, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> {
    return std::make_unique<env_engine>(); };

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { 1, 4 }));
  result.params.emplace_back(param_log(
    "{B1E6C162-07B6-4EE2-8EE1-EF5672FA86B4}", "A", 1, section_main, 0, 10, 1, 1, "Sec",
    param_dir::input, param_rate::accurate, param_edit::knob,
    param_label_contents::both, param_label_align::bottom, param_label_justify::center,
    gui_layout::single, gui_position { 0, 0 }));
  result.params.emplace_back(param_log(
    "{45E37229-839F-4735-A31D-07DE9873DF04}", "D", 1, section_main, 0, 10, 1, 1, "Sec",
    param_dir::input, param_rate::accurate, param_edit::knob,
    param_label_contents::both, param_label_align::bottom, param_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));
  result.params.emplace_back(param_pct(
    "{E5AB2431-1953-40E4-AFD3-735DB31A4A06}", "S", 1, section_main, 0, 1, 1,
    param_dir::input, param_rate::accurate, true, param_edit::knob,
    param_label_contents::both, param_label_align::bottom, param_label_justify::center,
    gui_layout::single, gui_position{ 0, 2 }));
  result.params.emplace_back(param_log(
    "{FFC3002C-C3C8-4C10-A86B-47416DF9B8B6}", "R", 1, section_main, 0, 10, 1, 1, "Sec",
    param_dir::input, param_rate::accurate, param_edit::knob,
    param_label_contents::both, param_label_align::bottom, param_label_justify::center,
    gui_layout::single, gui_position { 0, 3 }));

  return result;
}

void
env_engine::process(process_block& block)
{
}

}
