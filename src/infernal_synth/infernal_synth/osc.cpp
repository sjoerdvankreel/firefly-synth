#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { type_saw, type_sine };
enum { section_main, section_pitch, section_sine_gain, section_saw_gain };
enum { param_on, param_type, param_gain, param_bal, param_note, param_oct, param_cent, param_sine_gain, param_saw_gain };

static std::vector<std::string>
note_names()
{ return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; }

std::vector<list_item>
cv_matrix_target_osc_params(module_topo const& osc_topo)
{
  std::vector<list_item> result;
  result.emplace_back(osc_topo.params[param_bal]);
  result.emplace_back(osc_topo.params[param_gain]);
  result.emplace_back(osc_topo.params[param_cent]);
  return result;
}

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{E41F2F4B-7E80-4791-8E9C-CCE72A949DB6}", "Saw", type_saw);
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Sine", type_sine);
  return result;
}

class osc_engine:
public module_engine {
  float _phase;

public:
  osc_engine() { initialize(); }
  INF_PREVENT_ACCIDENTAL_COPY(osc_engine);
  void initialize() override { _phase = 0; }
  void process(plugin_block& block) override;
};

module_topo
osc_topo()
{ 
  module_topo result(make_module(
    make_topo_info("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Voice Osc", module_osc, 2), 
    make_module_dsp(module_stage::voice, module_output::audio, 1),
    make_module_gui(gui_layout::tabbed, { 2, 0 }, { 2, 4 })));
  result.engine_factory = [](int, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<osc_engine>(); };
  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{A64046EE-82EB-4C02-8387-4B9EFF69E06A}", "Main"),
    make_section_gui({ 1, 0, 1, 3 }, { { 1 }, { 1, 2, 2, 2 } })));
  auto& pitch = result.sections.emplace_back(make_section(section_pitch,
    make_topo_tag("{4CA0A189-9C44-4260-A5B5-B481527BD04A}", "Pitch"),
    make_section_gui({ 0, 0, 1, 4 }, { 1, 3 })));
  pitch.gui.bindings.enabled.params = { param_on };
  pitch.gui.bindings.enabled.selector = [](auto const& vs, auto const&) { return vs[0] != 0; };
  auto& sine_gain = result.sections.emplace_back(make_section(section_sine_gain,
    make_topo_tag("{A3D3BDF9-5CD0-47D2-BBDD-1BAD1274CDC4}", "Sine gain"),
    make_section_gui({ 1, 3, 1, 1 }, { 1, 1 })));
  sine_gain.gui.bindings.visible.params = { param_on, param_type };
  sine_gain.gui.bindings.visible.selector = [](auto const& vs, auto const&) { return vs[0] != 0 && vs[1] == type_sine; };
  auto& saw_gain = result.sections.emplace_back(make_section(section_saw_gain,
    make_topo_tag("{8CACFECE-A9D1-4B49-AEE6-015EAF5CA682}", "Saw gain"),
    make_section_gui( { 1, 3, 1, 1 }, { 1, 1 })));
  saw_gain.gui.bindings.visible.params = { param_on, param_type };
  saw_gain.gui.bindings.visible.selector = [](auto const& vs, auto const&) { return vs[0] != 0 && vs[1] == type_saw; };

  result.params.emplace_back(make_param(
    make_topo_info("{AA9D7DA6-A719-4FDA-9F2E-E00ABB784845}", "On", param_on, 1),
    make_param_dsp_block(), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 }, 
      make_label_default(gui_label_contents::name))));
  result.params.emplace_back(make_param(
    make_topo_info("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", param_type, 1),
    make_param_dsp_block(), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 1 }, 
      make_label_default(gui_label_contents::name))));
  result.params.emplace_back(make_param(
    make_topo_info("{75E49B1F-0601-4E62-81FD-D01D778EDCB5}", "Gain", param_gain, 1),
    make_param_dsp_accurate(), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 }, 
      make_label_default(gui_label_contents::name))));
  result.params.emplace_back(make_param(
    make_topo_info("{23C6BC03-0978-4582-981B-092D68338ADA}", "Bal", param_bal, 1),
    make_param_dsp_accurate(), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 3 }, 
      make_label_default(gui_label_contents::name))));
  result.params.emplace_back(make_param(
    make_topo_info("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", param_note, 1), 
    make_param_dsp_block(), make_domain_name(note_names(), ""),
    make_param_gui_single(section_pitch, gui_edit_type::list, { 0, 0 }, 
      make_label_none())));  
  result.params.emplace_back(make_param(
    make_topo_info("{38C78D40-840A-4EBE-A336-2C81D23B426D}", "Oct", param_oct, 1),
    make_param_dsp_block(), make_domain_step(0, 9, 4),
    make_param_gui_single(section_pitch, gui_edit_type::list, { 0, 1 }, 
      make_label_none())));  
  result.params.emplace_back(make_param(
    make_topo_info("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", param_cent, 1),
    make_param_dsp_accurate(), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_pitch, gui_edit_type::hslider, { 0, 2 }, 
      make_label(gui_label_contents::value, gui_label_align::right, gui_label_justify::center))));
  result.params.emplace_back(make_param(
    make_topo_info("{42E7A672-699C-4955-B45B-BBB8190A50E7}", "Sine Gain", param_sine_gain, 1),
    make_param_dsp_accurate(), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui_single(section_sine_gain, gui_edit_type::knob, { 0, 0 }, 
      make_label_none())));
  result.params.emplace_back(make_param(
    make_topo_info("{725B22B5-FAE9-4C4E-9B69-CAE46E4DCC6D}", "Saw Gain", param_saw_gain, 1),
    make_param_dsp_accurate(), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui_single(section_saw_gain, gui_edit_type::knob, { 0, 0 }, 
      make_label_none())));
  return result;
}

void
osc_engine::process(plugin_block& block)
{
  if(block.state.block_automation[param_on][0].step() == 0) return;
  int oct = block.state.block_automation[param_oct][0].step();
  int note = block.state.block_automation[param_note][0].step();
  int type = block.state.block_automation[param_type][0].step();
  auto const& env_curve = block.voice->cv_in[module_env][0][0];
  auto const& bal_curve = block.state.accurate_automation[param_bal][0];
  auto const& cent_curve = block.state.accurate_automation[param_cent][0];
  auto const& gain_curve = block.state.accurate_automation[param_gain][0];
  auto const& saw_gain_curve = block.state.accurate_automation[param_saw_gain][0];
  auto const& sine_gain_curve = block.state.accurate_automation[param_sine_gain][0];

  float sample;
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    switch (type)
    {
    case type_saw: sample = saw_gain_curve[f] * (_phase * 2 - 1); break;
    case type_sine: sample = sine_gain_curve[f] * std::sin(_phase * 2 * pi32); break;
    default: assert(false); sample = 0; break;
    }
    check_bipolar(sample);
    float bal = block.normalized_to_raw(module_osc, param_bal, bal_curve[f]);
    float cent = block.normalized_to_raw(module_osc, param_cent, cent_curve[f]);
    block.state.own_audio_out[0][0][f] = sample * gain_curve[f] * env_curve[f] * balance(0, bal);
    block.state.own_audio_out[0][1][f] = sample * gain_curve[f] * env_curve[f] * balance(1, bal);
    _phase += note_to_frequency(oct, note, cent, block.voice->state.id.key) / block.sample_rate;
    _phase -= std::floor(_phase);
  }
}

}
