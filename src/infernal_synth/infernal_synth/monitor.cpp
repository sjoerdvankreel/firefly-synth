#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { param_gain, param_voices, param_cpu, param_threads };

class monitor_engine: 
public module_engine {
public:
  monitor_engine() {}
  void initialize() override {}
  void process(plugin_block& block) override;
  INF_PREVENT_ACCIDENTAL_COPY(monitor_engine);
};

module_topo
monitor_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos, int polyphony)
{
  module_topo result(make_module(
    make_topo_info("{C20F2D2C-23C6-41BE-BFB3-DE9EDFB051EC}", "Monitor", module_monitor, 1), 
    make_module_dsp(module_stage::output, module_output::none, 0, {}),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{988E6A84-A012-413C-B33B-80B8B135D203}", "Main"),
    make_param_section_gui({ 0, 0 }, { 1, 4 })));

  result.params.emplace_back(make_param(
    make_topo_info("{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Gain", param_gain, 1),
    make_param_dsp_output(), make_domain_percentage(0, 9.99, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{2827FB67-CF08-4785-ACB2-F9200D6B03FA}", "Voices", param_voices, 1),
    make_param_dsp_output(), make_domain_step(0, polyphony, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{55919A34-BF81-4EDF-8222-F0F0BE52DB8E}", "Cpu", param_cpu, 1),
    make_param_dsp_output(), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{FD7E410D-D4A6-4AA2-BDA0-5B5E6EC3E13A}", "Thrds", param_threads, 1),
    make_param_dsp_output(), make_domain_step(0, polyphony, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<monitor_engine>(); };

  return result;
}

void
monitor_engine::process(plugin_block& block)
{
  float max_out = 0.0f;
  for (int c = 0; c < 2; c++)  
    for(int f = block.start_frame; f < block.end_frame; f++)
      max_out = std::max(max_out, block.out->host_audio[c][f]);
  block.set_out_param(param_voices, 0, block.out->voice_count);
  block.set_out_param(param_threads, 0, block.out->thread_count);
  block.set_out_param(param_cpu, 0, std::clamp(block.out->cpu_usage, 0.0, 1.0));
  block.set_out_param(param_gain, 0, std::clamp(max_out, 0.0f, (float)block.plugin.modules[module_monitor].params[param_gain].domain.max));
}

}
