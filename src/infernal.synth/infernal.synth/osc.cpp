#include <infernal.base/topo.hpp>
#include <infernal.synth/synth.hpp>

using namespace infernal::base;

namespace infernal::synth {

enum osc_type { osc_type_saw, osc_type_sine };
enum osc_param { osc_param_on, osc_param_gain, osc_param_bal, osc_param_oct, osc_param_note, osc_param_cent };

module_group_topo
osc_topo()
{
  module_group_topo result = {};
  result.module_count = 2;
  result.name = "Osc";
  result.scope = module_scope::voice;
  result.output = module_output::audio;
  result.id = "{45C2CCFE-48D9-4231-A327-319DAE5C9366}";
  
  param_topo on;
  on.default_text

  return result;
}

}
