#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

class filter_engine: 
public module_engine {  
  float _in[2];
  float _out[2];
public:
  filter_engine() { reset(); }
  INF_DECLARE_MOVE_ONLY(filter_engine);
  void reset() override { _in[0] = _in[1] = _out[0] = _out[1] = 0; }
  void process(process_block& block, int start_frame, int end_frame) override;
};

enum { section_main };
enum { param_on, param_freq, param_osc_gain };

module_topo
filter_topo(int osc_slot_count)
{
  module_topo result(make_module(
    "{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Filter", 1, 
    module_stage::voice, module_output::none,
    gui_layout::single, gui_position { 1, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> {
    return std::make_unique<filter_engine>(); };

  std::vector<int> column_sizes;
  column_sizes.push_back(1);
  column_sizes.push_back(2);
  for(int i = 0; i < osc_slot_count; i++) column_sizes.push_back(2);

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { { 1 }, column_sizes }));
  result.params.emplace_back(param_toggle(
    "{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", 1, section_main, false,
    param_dir::input,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 0 }));
  result.params.emplace_back(param_log(
    "{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", 1, section_main, 20, 20000, 1000, 1000, "Hz",
    param_dir::input, param_rate::accurate, param_edit::hslider,
    param_label_contents::name, param_label_align::bottom, param_label_justify::near,
    gui_layout::single, gui_position { 0, 1 }));
  result.params.emplace_back(param_pct(
    "{B377EBB2-73E2-46F4-A2D6-867693ED9ACE}", "Osc Gain", osc_slot_count, section_main, 0, 1, 0.5,
    param_dir::input, param_rate::accurate, true, param_edit::hslider,
    param_label_contents::name, param_label_align::bottom, param_label_justify::near,
    gui_layout::horizontal, gui_position { 0, 2, 1, osc_slot_count }));

  return result;
}

void
filter_engine::process(process_block& block, int start_frame, int end_frame)
{
  auto const& osc_audio = block.voice->audio_in[module_osc];
  auto const& osc_gain = block.accurate_automation[param_osc_gain];
  for(int o = 0; o < block.plugin.modules[module_osc].slot_count; o++)
    for(int c = 0; c < 2; c++)
      for(int f = start_frame; f < end_frame; f++)
        block.voice->result[c][f] += osc_audio[o][c][f] * osc_gain[o][f];
  if(block.block_automation[param_on][0].step() == 0) return;

  float w = 2 * block.sample_rate;
  auto const& freq = block.accurate_automation[param_freq][0];
  for (int f = start_frame; f < end_frame; f++)
  {
    float angle = freq[f] * 2 * pi32;
    float norm = 1 / (angle + w);
    float a = angle * norm;
    float b = (w - angle) * norm;
    for (int c = 0; c < 2; c++)
    {
      float filtered = block.voice->result[c][f] * a + _in[c] * a + _out[c] * b;
      _in[c] = block.voice->result[c][f];
      _out[c] = filtered;
      block.voice->result[c][f] = filtered;
    }
  }
}

}
