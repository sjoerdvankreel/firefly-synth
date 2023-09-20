#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

class osc_engine: 
public module_engine {
  float _phase = 0;
public:
  INF_DECLARE_MOVE_ONLY(osc_engine);
  void process(process_block& block) override;
};

static std::vector<item_topo>
type_items()
{
  std::vector<item_topo> result;
  result.emplace_back("{E41F2F4B-7E80-4791-8E9C-CCE72A949DB6}", "Saw");
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Sine");
  return result;
}

enum { type_saw, type_sine };
enum { section_main, section_pitch };
enum { param_on, param_type, param_gain, param_bal, param_oct, param_note, param_cent };

module_topo
osc_topo()
{
  module_topo result(make_module(
    "{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Osc", 2, 
    module_stage::voice, module_output::audio, 
    gui_layout::tabbed, gui_position { 0, 0 }, gui_dimension { 2, 1 }));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> {
    return std::make_unique<osc_engine>(); };

  result.sections.emplace_back(make_section(
    "Main", section_main, gui_position { 0, 0 }, gui_dimension { { 1 }, { 1, 2, 2, 2 } }));
  result.params.emplace_back(param_toggle(
    "{AA9D7DA6-A719-4FDA-9F2E-E00ABB784845}", "On", 1, section_main, false, 
    param_dir::input, 
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 0 }));
  result.params.emplace_back(param_items(
    "{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", 1, section_main, type_items(), "Saw",
    param_dir::input, param_edit::list, 
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));
  result.params.emplace_back(param_pct(
    "{75E49B1F-0601-4E62-81FD-D01D778EDCB5}", "Gain", 1, section_main, 0, 1, 1, 
    param_dir::input, param_rate::accurate, true, param_edit::hslider,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 2 }));
  result.params.emplace_back(param_pct(
    "{23C6BC03-0978-4582-981B-092D68338ADA}", "Bal", 1, section_main, -1, 1, 0, 
    param_dir::input, param_rate::accurate, true, param_edit::hslider,
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 3 }));

  result.sections.emplace_back(make_section(
    "Pitch", section_pitch, gui_position{ 1, 0 }, gui_dimension { 1, 3 }));
  result.params.emplace_back(param_steps(
    "{38C78D40-840A-4EBE-A336-2C81D23B426D}", "Oct", 1, section_pitch, 0, 9, 4,
    param_dir::input, param_edit::list, 
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 0 }));
  result.params.emplace_back(param_names(
    "{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", 1, section_pitch, note_names(), "C", 
    param_dir::input, param_edit::text, 
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 1 }));
  result.params.emplace_back(param_pct(
    "{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", 1, section_pitch, -1, 1, 0,
    param_dir::input, param_rate::accurate, false, param_edit::knob, 
    param_label_contents::name, param_label_align::left, param_label_justify::center,
    gui_layout::single, gui_position { 0, 2 }));

  return result;
}

void
osc_engine::process(process_block& block)
{
  if(block.block_automation[param_on][0].step() == 0) return;
  int oct = block.block_automation[param_oct][0].step();
  int note = block.block_automation[param_note][0].step();
  int type = block.block_automation[param_type][0].step();
  auto const& bal = block.accurate_automation[param_bal][0];
  auto const& cent = block.accurate_automation[param_cent][0];
  auto const& gain = block.accurate_automation[param_gain][0];

  float sample;
  for (int f = 0; f < block.host.frame_count; f++)
  {
    switch (type)
    {
    case type_saw: sample = _phase * 2 - 1; break;
    case type_sine: sample = std::sin(_phase * 2 * pi32); break;
    default: assert(false); sample = 0; break;
    }
    block.audio_out[0][f] = sample * gain[f] * balance(0, bal[f]);
    block.audio_out[1][f] = sample * gain[f] * balance(1, bal[f]);
    _phase += note_to_frequency(oct, note, cent[f], block.voice->state.id.key) / block.sample_rate;
    _phase -= std::floor(_phase);
  }
}

}
