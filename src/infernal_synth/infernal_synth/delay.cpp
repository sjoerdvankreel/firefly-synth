#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>

#include <array>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

class delay_engine: 
public module_engine {
  int _pos = 0;
  int const _length;
  jarray<float, 2> _buffer = {};
public:
  INF_DECLARE_MOVE_ONLY(delay_engine);
  delay_engine(int sample_rate);
  void process(process_block& block) override;
};

enum { section_main };
enum { param_on, param_out_gain };

module_topo
delay_topo()
{
  module_topo result(make_module(
    "{ADA77C05-5D2B-4AA0-B705-A5BE89C32F37}", "Delay", 1, 
    module_stage::output, module_output::none,
    gui_layout::single, gui_position { 2, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> {
    return std::make_unique<delay_engine>(sample_rate); };

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { 1, 2 }));
  result.params.emplace_back(param_toggle(
    "{A8638DE3-B574-4584-99A2-EC6AEE725839}", "On", 1, 
    section_main, param_dir::input, param_label::default_, false,
    gui_layout::single, gui_position { 0, 0 } ));
  result.params.emplace_back(param_pct(
    "{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Out Gain", 1,
    section_main, param_dir::output, param_edit::text, param_label::default_, param_rate::block, true, 0, 1, 0,
    gui_layout::single, gui_position { 0, 1 }));

  return result;
}

delay_engine::
delay_engine(int sample_rate) : 
_length(sample_rate / 2)
{ _buffer.resize(jarray<int, 1>(2, _length)); }

void
delay_engine::process(process_block& block)
{
  float max_out = 0.0f;
  for (int c = 0; c < 2; c++)  
    for(int f = 0; f < block.host.frame_count; f++)
    {
      block.out->host_audio[c][f] = block.out->mixdown[c][f];
      if (block.block_automation[param_on][0].step() != 0)
        block.out->host_audio[c][f] += _buffer[c][(_pos + f) % _length];
      _buffer[c][(_pos + f) % _length] = block.out->mixdown[c][f];
      max_out = std::max(max_out, block.out->host_audio[c][f]);
    }  
  _pos += block.host.frame_count;
  _pos %= _length;
  block.set_out_param(param_out_gain, 0, std::clamp(max_out, 0.0f, 1.0f));
}

}
