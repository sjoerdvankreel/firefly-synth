#include <infernal.base/plugin_topo.hpp>
#include <infernal.base/plugin_support.hpp>

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
static_topo(topo)
{
  int plugin_param_index = 0;
  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& mod = topo.modules[m];
    flat_module_topo flat;
    flat.static_topo = mod;
    for (int s = 0; s < mod.submodules.size(); s++)
    {
      auto const& submod = mod.submodules[s];
      for (int p = 0; p < submod.params.size(); p++)
        flat.params.push_back(submod.params[p]);
    }
    flat_modules.push_back(flat);

    for(int i = 0; i < mod.count; i++)
    {
      runtime_module_topo rt_module;
      rt_module.static_topo = mod;
      rt_module.name = mod.name;
      if(mod.count > 1) rt_module.name += " " + std::to_string(i);

      int mod_param_index = 0;
      for(int s = 0; s < mod.submodules.size(); s++)
      {
        auto const& submod = mod.submodules[s];
        for (int p = 0; p < submod.params.size(); p++)
        {
          runtime_param_topo rtp;
          auto const& param = submod.params[p];
          rtp.static_topo = param;
          rtp.module_index = i;
          rtp.module_param_index = mod_param_index++;
          rtp.name = rt_module.name + " " + param.name;
          rtp.id = mod.id + std::to_string(i) + param.id;
          rtp.id_hash = stable_hash_nonnegative(rtp.id.c_str());
          rt_module.params.push_back(rtp);
        }
      }
      runtime_modules.push_back(rt_module);
    }
  }
}

}