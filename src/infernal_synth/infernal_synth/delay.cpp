#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <array>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { param_on, param_out, param_voices, param_cpu, param_threads };

class delay_engine: 
public module_engine {
  int _pos;
  int const _length;
  jarray<float, 2> _buffer = {};

public:
  delay_engine(int sample_rate);
  INF_PREVENT_ACCIDENTAL_COPY(delay_engine);
  void initialize() override;
  void process(plugin_block& block) override;
};

module_topo
delay_topo(plugin_base::gui_position const& pos, int polyphony)
{
  module_topo result(make_module(
    make_topo_info("{ADA77C05-5D2B-4AA0-B705-A5BE89C32F37}", "Delay", module_delay, 1), 
    make_module_dsp(module_stage::output, module_output::none, 0, 0),
    make_module_gui(gui_layout::single, pos, { 1, 1 })));
  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{05CF51D6-35F9-4115-A654-83EEE584B68E}", "Main"),
    make_section_gui({ 0, 0 }, { 1, 5 })));
  result.engine_factory = [](auto const&, int sample_rate, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<delay_engine>(sample_rate); };

  result.params.emplace_back(make_param(
    make_topo_info("{A8638DE3-B574-4584-99A2-EC6AEE725839}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 }, 
      make_label_default(gui_label_contents::name))));
  result.params.emplace_back(make_param(
    make_topo_info("{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Out", param_out, 1),
    make_param_dsp_output(), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::text, { 0, 1 }, 
      make_label_default(gui_label_contents::name))));  
  result.params.emplace_back(make_param(
    make_topo_info("{2827FB67-CF08-4785-ACB2-F9200D6B03FA}", "Voices", param_voices, 1),
    make_param_dsp_output(), make_domain_step(0, polyphony, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 2 }, 
      make_label_default(gui_label_contents::name))));  
  result.params.emplace_back(make_param(
    make_topo_info("{55919A34-BF81-4EDF-8222-F0F0BE52DB8E}", "Cpu", param_cpu, 1),
    make_param_dsp_output(), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::text, { 0, 3 }, 
      make_label_default(gui_label_contents::name))));  
  result.params.emplace_back(make_param(
    make_topo_info("{FD7E410D-D4A6-4AA2-BDA0-5B5E6EC3E13A}", "Threads", param_threads, 1),
    make_param_dsp_output(), make_domain_step(0, polyphony, 0, 0),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 4 }, 
      make_label_default(gui_label_contents::name))));
  return result;
}

delay_engine::
delay_engine(int sample_rate) : 
_length(sample_rate / 2)
{ 
  _buffer.resize(jarray<int, 1>(2, _length)); 
  initialize();
}

void
delay_engine::initialize()
{
  _pos = 0;
  _buffer.fill(0);
}

void
delay_engine::process(plugin_block& block)
{
  float max_out = 0.0f;
  for (int c = 0; c < 2; c++)  
    for(int f = block.start_frame; f < block.end_frame; f++)
    {
      block.out->host_audio[c][f] = block.out->mixdown[c][f];
      if (block.state.own_block_automation[param_on][0].step() != 0)
        block.out->host_audio[c][f] += _buffer[c][(_pos + f) % _length] * 0.5f;
      _buffer[c][(_pos + f) % _length] = block.out->mixdown[c][f];
      max_out = std::max(max_out, block.out->host_audio[c][f]);
    }  
  _pos += block.end_frame - block.start_frame;
  _pos %= _length;
  block.set_out_param(param_voices, 0, block.out->voice_count);
  block.set_out_param(param_threads, 0, block.out->thread_count);
  block.set_out_param(param_out, 0, std::clamp(max_out, 0.0f, 1.0f));
  block.set_out_param(param_cpu, 0, std::clamp(block.out->cpu_usage, 0.0, 1.0));
}

}
