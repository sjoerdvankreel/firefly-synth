#include <infernal.base/topo.hpp>
#include <infernal.base/support.hpp>
#include <infernal.synth/synth.hpp>

using namespace infernal::base;

namespace infernal::synth {

enum osc_type { osc_type_saw, osc_type_sine };
enum osc_param { osc_param_on, osc_param_gain, osc_param_bal, osc_param_oct, osc_param_note, osc_param_cent };

module_group_topo
osc_topo()
{
  module_group_topo result(make_module_group("{45C2CCFE-48D9-4231-A327-319DAE5C9366}", "Osc", 2, module_scope::voice, module_output::audio));
  result.params.emplace_back(make_param_toggle("{AA9D7DA6-A719-4FDA-9F2E-E00ABB784845}", "On", "Off", param_direction::input));
  result.params.emplace_back(make_param_pct("{75E49B1F-0601-4E62-81FD-D01D778EDCB5}", "Gain", "100", 0, 1, param_direction::input, param_rate::accurate, param_display::knob));
  result.params.emplace_back(make_param_pct("{23C6BC03-0978-4582-981B-092D68338ADA}", "Bal", "0", -1, 1, param_direction::input, param_rate::accurate, param_display::knob));
  result.params.emplace_back(make_param_step("{38C78D40-840A-4EBE-A336-2C81D23B426D}", "Oct", "0", 0, 9, param_direction::input, param_display::knob));
  result.params.emplace_back(make_param_list("{78856BE3-31E2-4E06-A6DF-2C9BB534789F}", "Note", "0", note_name_items(), param_direction::input, param_display::knob));
  result.params.emplace_back(make_param_pct("{691F82E5-00C8-4962-89FE-9862092131CB}", "Cent", "0", -1, 1, param_direction::input, param_rate::accurate, param_display::knob));
  return result;
}

}
