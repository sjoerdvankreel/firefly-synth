#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <firefly_synth/synth.hpp>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth {

enum { section_main };
enum { param_gain, param_cpu, param_hi_mod_cpu, param_hi_mod, param_voices, param_threads };

class monitor_engine: 
public module_engine {
  bool const _is_fx;
public:
  void process(plugin_block& block) override;
  void reset(plugin_block const*) override {}
  monitor_engine(bool is_fx) : _is_fx(is_fx) {}
  PB_PREVENT_ACCIDENTAL_COPY(monitor_engine);
};

module_topo
monitor_topo(int section, gui_colors const& colors, gui_position const& pos, int polyphony, bool is_fx)
{
  module_topo result(make_module(
    make_topo_info_basic("{C20F2D2C-23C6-41BE-BFB3-DE9EDFB051EC}", "Monitor", module_monitor, 1),
    make_module_dsp(module_stage::output, module_output::none, 0, {}),
    make_module_gui(section, colors, pos, { 1, 1 })));
  result.info.description = "Monitor module with active voice count, CLAP threadpool thread count, master gain, overall CPU usage and highest-module CPU usage.";
  
  result.gui.enable_tab_menu = false;
  result.engine_factory = [is_fx](auto const&, int, int) { return std::make_unique<monitor_engine>(is_fx); };

  int column_hoffset = is_fx? -2: 0;
  std::vector<int> column_distribution = { gui_dimension::auto_size, gui_dimension::auto_size, gui_dimension::auto_size, 1 };
  if(!is_fx) column_distribution.insert(column_distribution.begin(), { gui_dimension::auto_size, gui_dimension::auto_size });
  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{988E6A84-A012-413C-B33B-80B8B135D203}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 } , column_distribution })));
  auto& gain = result.params.emplace_back(make_param(
    make_topo_info_basic("{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Gain", param_gain, 1),
    make_param_dsp_output(), make_domain_percentage(0, 9.99, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 2 + column_hoffset },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  gain.info.description = "Master output gain. Nothing is clipped, so this may well exceed 100%.";
  auto& cpu = result.params.emplace_back(make_param(
    make_topo_info_basic("{55919A34-BF81-4EDF-8222-F0F0BE52DB8E}", "CPU", param_cpu, 1),
    make_param_dsp_output(), make_domain_percentage(0, 9.99, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 3 + column_hoffset },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  cpu.info.description = std::string("CPU usage relative to last processing block length. ") + 
    "For example, if it took 1 ms to render a 5 ms block, this will be 20%.";
  auto& hi_cpu = result.params.emplace_back(make_param(
    make_topo_info_basic("{2B13D43C-FFB5-4A66-9532-39B0F8258161}", "High CPU", param_hi_mod_cpu, 1),
    make_param_dsp_output(), make_domain_percentage(0, 0.99, 0, 0, false),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 4 + column_hoffset },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  hi_cpu.info.description = "CPU usage of the most expensive module, relative to total CPU usage.";
  auto& hi_module = result.params.emplace_back(make_param(
    make_topo_info_basic("{BE8AF913-E888-4A0E-B674-8151AF1B7D65}", "High CPU Module", param_hi_mod, 1),
    make_param_dsp_output(), make_domain_step(0, 999, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::output_module_name, { 0, 5 + column_hoffset },
      make_label(gui_label_contents::none, gui_label_align::left, gui_label_justify::center))));
  hi_module.info.description = "Module that used the most CPU relative to total usage.";
  
  if(is_fx) return result;

  auto& voices = result.params.emplace_back(make_param(
    make_topo_info_basic("{2827FB67-CF08-4785-ACB2-F9200D6B03FA}", "Voices", param_voices, 1),
    make_param_dsp_output(), make_domain_step(0, polyphony, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  voices.info.description = "Active voice count. Max 32, after that, recycling will occur.";
  auto& thrs = result.params.emplace_back(make_param(
    make_topo_info_basic("{FD7E410D-D4A6-4AA2-BDA0-5B5E6EC3E13A}", "Threads", param_threads, 1),
    make_param_dsp_output(), make_domain_step(0, polyphony, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::output, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  thrs.info.description = "Number of CLAP threadpool threads used to process voices in the last block. For VST3, this will always be 0 or 1.";
  return result;
}

void
monitor_engine::process(plugin_block& block)
{
  float max_out = 0.0f;
  for (int c = 0; c < 2; c++)  
    for(int f = block.start_frame; f < block.end_frame; f++)
      max_out = std::max(max_out, block.out->host_audio[c][f]);

  auto const& params = block.plugin.modules[module_monitor].params;
  if(!_is_fx)
  {
    block.set_out_param(param_voices, 0, block.out->voice_count);
    block.set_out_param(param_threads, 0, block.out->thread_count);
  }
  block.set_out_param(param_hi_mod, 0, block.out->high_cpu_module);
  block.set_out_param(param_gain, 0, std::clamp((double)max_out, 0.0, params[param_gain].domain.max));
  block.set_out_param(param_cpu, 0, std::clamp(block.out->cpu_usage, 0.0, params[param_cpu].domain.max));
  block.set_out_param(param_hi_mod_cpu, 0, std::clamp(block.out->high_cpu_module_usage, 0.0, params[param_hi_mod_cpu].domain.max));
}

}
