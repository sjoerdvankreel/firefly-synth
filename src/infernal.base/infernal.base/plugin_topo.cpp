#include <infernal.base/plugin_topo.hpp>

namespace infernal::base {

param_value
param_topo::default_value() const
{
  param_value value;
  value.step = 0;
  return value;
}

std::string 
param_topo::to_text(param_value value) const
{
  return "0";
}

bool
param_topo::from_text(std::string const& text, param_value& value) const
{
  value.step = 0;
  return true;
}

runtime_plugin_topo::
runtime_plugin_topo(plugin_topo const& topo):
plugin_topo(topo)
{
  for (int m = 0; m < topo.modules.size(); m++)
  {
    for (int s = 0; s < topo.modules[m].submodules.size(); s++)
      for(int p = 0; p < topo.modules[m].submodules[s].params.size(); p++)
        module_params[m].push_back(topo.modules[m].submodules[s].params[p]);
    for(int i = 0; i < topo.modules[m].count; i++)
      for (int s = 0; s < topo.modules[m].submodules.size(); s++)
      {
        runtime_param_topo runtime_param;
        runtime_param.module_type = m;
        runtime_param.module_index = i;
        runtime_param.module_param_index = module_param_index++;
        runtime_params.push_back(runtime_param);
      }
  }
}

}