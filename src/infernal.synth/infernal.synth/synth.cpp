#include <infernal.synth/synth_topo.hpp>

using namespace infernal::base;

namespace infernal::synth {

static module_topo 
osc_topo()
{
  module_topo result = {};
  osc.count = 2;
  osc.name = "Osc";
  osc.type = module_type_osc;
  osc.scope = module_scope::voice;
  osc.output = module_output::audio;
  osc.id = "{45C2CCFE-48D9-4231-A327-319DAE5C9366}";
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
  osc_pitch.name = ""

}

}
