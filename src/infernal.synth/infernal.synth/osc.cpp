#include <infernal.synth/synth.hpp>
#include <infernal.base/plugin_topo.hpp>
#include <infernal.base/plugin_support.hpp>

using namespace infernal::base;

namespace infernal::synth {

enum osc_type { osc_type_saw, osc_type_sine };
enum osc_param { osc_param_on, osc_param_gain, osc_param_bal, osc_param_oct, osc_param_note, osc_param_cent };

submodule_topo
main_topo()
{
  submodule_topo result;
  result.name = "Main";
  
  param_topo on = toggle_topo();
  on.name = "On";
  on.default_ = "Off";
  on.type = osc_param_on;
  on.id = "{031051C7-8CAC-4ECD-AC44-3BCD3CCACA97}";
  result.params.emplace_back(std::move(on));

  return {};
}

module_topo
osc_topo()
{
  module_topo result = {};
  result.count = 2;
  result.name = "Osc";
  result.type = module_type_osc;
  result.scope = module_scope::voice;
  result.output = module_output::audio;
  result.id = "{45C2CCFE-48D9-4231-A327-319DAE5C9366}";
  return result;
}

}
