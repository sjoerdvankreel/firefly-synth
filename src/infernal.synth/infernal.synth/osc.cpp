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
  result.params.emplace_back(make_param_toggle());
  return result;
}

}
