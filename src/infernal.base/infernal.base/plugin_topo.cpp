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
    std::vector<param_topo> flat_params;
    for (int s = 0; s < topo.modules[m].submodules.size(); s++)
      for (int p = 0; p < topo.modules[m].submodules[s].params.size(); p++)
        flat_params.push_back(topo.modules[m].submodules[s].params[p]);
    flat_module_params.push_back(flat_params);

    runtime_param_topo runtime_param;
    runtime_param.module_type = m;
    runtime_module_topo runtime_module;
    runtime_module.name = topo.modules[m].name;
    for (int i = 0; i < topo.modules[m].count; i++)
    {
      int module_param_index = 0;
      runtime_param.module_index = i;
      if (topo.modules[m].count > 1)
        runtime_module.name = topo.modules[m].name + std::string(" ") + std::to_string(i + 1);
      for (int s = 0; s < topo.modules[m].submodules.size(); s++)
        for (int p = 0; p < topo.modules[m].submodules[s].params.size(); p++)
        {
          runtime_param.module_param_index = module_param_index++;
          runtime_param.name = runtime_module.name + " " + topo.modules[m].submodules[s].params[p].name;
          runtime_params.push_back(runtime_param);
        }
      runtime_modules.push_back(runtime_module);
    }
  }
}

}