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

enum { type_saw, type_sine };
enum { section_main, section_pitch };
enum { param_on, param_type, param_gain, param_bal, param_oct, param_note, param_cent };

module_topo
osc_topo()
{
  module_topo result(make_module("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Osc", 2, 
    module_scope::voice, module_output::audio));
  result.sections.emplace_back(section_topo(section_main, "Main"));
  result.sections.emplace_back(section_topo(section_pitch, "Pitch"));
  result.params.emplace_back(param_toggle("{AA9D7DA6-A719-4FDA-9F2E-E00ABB784845}", "On", 1, 
    section_main, param_dir::input, param_label::both, false));
  result.params.emplace_back(param_items("{960D3483-4B3E-47FD-B1C5-ACB29F15E78D}", "Type", 1, 
    section_main, param_dir::input, param_edit::list, param_label::both, type_items(), "Saw"));
  result.params.emplace_back(param_pct("{75E49B1F-0601-4E62-81FD-D01D778EDCB5}", "Gain", 1, 
    section_main, param_dir::input, param_edit::knob, param_label::both, param_rate::accurate, true, 0, 1, 1));
  result.params.emplace_back(param_pct("{23C6BC03-0978-4582-981B-092D68338ADA}", "Bal", 1, 
    section_pitch, param_dir::input, param_edit::knob, param_label::both, param_rate::accurate, true, -1, 1, 0));
  result.params.emplace_back(param_steps("{38C78D40-840A-4EBE-A336-2C81D23B426D}", "Oct", 1, 
    section_pitch, param_dir::input, param_edit::list, param_label::both, 0, 9, 4));
  result.params.emplace_back(param_names("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", 1, 
    section_pitch, param_dir::input, param_edit::text, param_label::both, note_names(), "C"));
  result.params.emplace_back(param_pct("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", 1, 
    section_pitch, param_dir::input, param_edit::knob, param_label::both, param_rate::accurate, false, -1, 1, 0));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<osc_engine>(); };
  return result;
}

void
osc_engine::process(
  plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  if(module.in.block()[param_on][0].step() == 0) return;
  int oct = module.in.block()[param_oct][0].step();
  int note = module.in.block()[param_note][0].step();
  int type = module.in.block()[param_type][0].step();
  auto const& bal = module.in.accurate()[param_bal][0];
  auto const& cent = module.in.accurate()[param_cent][0];
  auto const& gain = module.in.accurate()[param_gain][0];

  float sample;
  for (int f = 0; f < plugin.host->frame_count; f++)
  {
    switch (type)
    {
    case type_saw: sample = _phase * 2 - 1; break;
    case type_sine: sample = std::sin(_phase * 2 * pi32); break;
    default: assert(false); sample = 0; break;
    }
    module.out.audio()[0][f] = sample * gain[f] * balance(0, bal[f]);
    module.out.audio()[1][f] = sample * gain[f] * balance(1, bal[f]);
    _phase += note_to_frequency(oct, note, cent[f], module.in.voice->key) / plugin.sample_rate;
    _phase -= std::floor(_phase);
  }
}

}
