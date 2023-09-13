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
  void process(
    plugin_topo const& topo, 
    plugin_block const& plugin,
    module_block& module) override;
};

static std::vector<item_topo>
type_items()
{
  std::vector<item_topo> result;
  result.emplace_back("{E41F2F4B-7E80-4791-8E9C-CCE72A949DB6}", "Saw");
  result.emplace_back("{9185A6F4-F9EF-4A33-8462-1B02A25FDF29}", "Sine");
  return result;
}

enum osc_type { osc_type_saw, osc_type_sine };
enum osc_group { osc_group_main, osc_group_pitch };
enum osc_param { osc_param_on, osc_param_type, osc_param_gain, osc_param_bal, osc_param_oct, osc_param_note, osc_param_cent };

module_group_topo
osc_topo()
{
  module_group_topo result(make_module_group("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Osc", 2, module_scope::voice, module_output::audio));
  result.param_groups.emplace_back(param_group_topo(osc_group_main, "Main"));
  result.param_groups.emplace_back(param_group_topo(osc_group_pitch, "Pitch"));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<osc_engine>(); };
  result.params.emplace_back(param_toggle("{AA9D7DA6-A719-4FDA-9F2E-E00ABB784845}", "On", osc_group_main, param_direction::input, param_label::both, false));
  result.params.emplace_back(param_items("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", osc_group_main, param_direction::input, param_edit::list, param_label::both, type_items, "Saw"));
  result.params.emplace_back(param_percentage("{75E49B1F-0601-4E62-81FD-D01D778EDCB5}", "Gain", osc_group_main, param_direction::input, param_edit::knob, param_label::both, param_rate::accurate, true, 0, 1, 1));
  result.params.emplace_back(param_percentage("{23C6BC03-0978-4582-981B-092D68338ADA}", "Bal", osc_group_pitch, param_direction::input, param_edit::knob, param_label::both, param_rate::accurate, true, -1, 1, 0));
  result.params.emplace_back(param_steps("{38C78D40-840A-4EBE-A336-2C81D23B426D}", "Oct", osc_group_pitch, param_direction::input, param_edit::list, param_label::both, 0, 9, 4));
  result.params.emplace_back(param_names("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", osc_group_pitch, param_direction::input, param_edit::text, param_label::both, note_names(), "C"));
  result.params.emplace_back(param_percentage("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", osc_group_pitch, param_direction::input, param_edit::knob, param_label::both, param_rate::accurate, false, -1, 1, 0));
  return result;
}

void
osc_engine::process(
  plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  int on = module.block_automation[osc_param_on].step();
  if (!on) return;

  int oct = module.block_automation[osc_param_oct].step();
  int note = module.block_automation[osc_param_note].step();
  int type = module.block_automation[osc_param_type].step();
  auto const& bal_curve = module.accurate_automation[osc_param_bal];
  auto const& cent_curve = module.accurate_automation[osc_param_cent];
  auto const& gain_curve = module.accurate_automation[osc_param_gain];

  for (int f = 0; f < plugin.host->frame_count; f++)
  {
    float sample;
    switch (type)
    {
    case osc_type_saw: sample = _phase * 2 - 1; break;
    case osc_type_sine: sample = std::sin(_phase * 2.0f * INF_PI); break;
    default: assert(false); sample = 0; break;
    }
    module.audio_output[0][f] = sample * gain_curve[f] * balance(0, bal_curve[f]);
    module.audio_output[1][f] = sample * gain_curve[f] * balance(1, bal_curve[f]);
    _phase += note_to_frequency(oct, note, cent_curve[f]) / plugin.sample_rate;
    _phase -= std::floor(_phase);
  }
}

}
