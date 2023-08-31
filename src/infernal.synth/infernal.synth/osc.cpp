#include <infernal.synth/synth.hpp>
#include <infernal.base/plugin_topo.hpp>

using namespace infernal::base;

namespace infernal::synth {

enum osc_type { osc_type_saw, osc_type_sine };
enum osc_param { osc_param_on, osc_param_gain, osc_param_bal, osc_param_oct, osc_param_note, osc_param_cent };

static submodule_topo
main_topo()
{
  submodule_topo result;
  result.name = "Main";
  
  param_topo on;
  on.unit = "";
  on.name = "On";
  on.default_ = "Off";
  on.stepped_min = 0;
  on.stepped_max = 1;
  on.type = osc_param_on;
  on.rate = param_rate::block;
  on.slope = param_slope::linear;
  on.format = param_format::step;
  on.storage = param_storage::num;
  on.display = param_display::toggle;
  on.direction = param_direction::input;
  on.id = "{031051C7-8CAC-4ECD-AC44-3BCD3CCACA97}";
  result.params.emplace_back(std::move(on));
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
  result.callbacks = {};
  result.dependencies = {};
  result.submodules = {};
  return result;
}

plugin_topo
synth_topo()
{
  plugin_topo result;

  module_topo osc = {};
  osc.count = 2;
  osc.name = "Osc";
  osc.type = module_type_osc;
  osc.scope = module_scope::voice;
  osc.output = module_output::audio;
  osc.id = "{45C2CCFE-48D9-4231-A327-319DAE5C9366}";
  
  submodule_topo osc_main = {};
  osc_main.name = "Amp";
  param_topo osc_main_gain = {};
  osc_main_gain.default_ = "0";
  osc_main_gain.format = param_format::real;
  osc_main_gain.display = param_display::knob;
  osc_main_gain.direction = param_direction::input;
  osc_main_gain.id = "{F4F686D6-83DD-4E29-B633-4593C85FD738}";
  osc_main_gain.name = "Gain";
  osc_main_gain.rate = param_rate::accurate;

  submodule_topo osc_pitch = {};
  return result;
}

}
