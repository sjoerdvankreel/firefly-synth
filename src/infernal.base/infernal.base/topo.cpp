#include <infernal.base/plugin_topo.hpp>
#include <utility>

namespace infernal::base {

static int
hash(char const* text)
{
  std::uint32_t h = 0;
  int const multiplier = 33;
  auto utext = reinterpret_cast<std::uint8_t const*>(text);
  for (auto const* p = utext; *p != '\0'; p++) h = multiplier * h + *p;
  return std::abs(static_cast<int>(h + (h >> 5)));
}



param_value default_value() const;
double to_normalized(para_value value) const;
std::string to_text(param_value value) const;
param_value from_normalized(double normalized) const;
bool from_text(std::string const& text, param_value& value) const;

runtime_plugin_topo::
runtime_plugin_topo(plugin_topo&& static_topo_):
static_topo(std::move(static_topo_))
{
  for (int m = 0; m < static_topo.modules.size(); m++)
  {
    auto& flat = flat_modules[m];
    auto const& mod = static_topo.modules[m];
    flat.static_topo = &mod;
    for (int s = 0; s < mod.submodules.size(); s++)
    {
      auto const& submod = mod.submodules[s];
      for (int p = 0; p < submod.params.size(); p++)
        flat.params.push_back(&submod.params[p]);
    }
    flat_modules.emplace_back(std::move(flat));

    for(int i = 0; i < mod.count; i++)
    {
      runtime_module_topo rt_module;
      rt_module.static_topo = &mod;
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
          rtp.static_topo = &param;
          rtp.module_index = i;
          rtp.id_hash = hash(rtp.id.c_str());
          rtp.module_param_index = mod_param_index++;
          rtp.name = rt_module.name + " " + param.name;
          rtp.id = mod.id + std::to_string(i) + param.id;
          rt_module.params.emplace_back(std::move(rtp));
        }
      }
      runtime_modules.emplace_back(std::move(rt_module));
    }
  }
}

}