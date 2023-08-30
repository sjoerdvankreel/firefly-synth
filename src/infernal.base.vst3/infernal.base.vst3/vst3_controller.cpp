#include <infernal.base.vst3/vst3_controller.hpp>
#include <infernal.base.vst3/vst3_support.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

tresult PLUGIN_API 
vst3_controller::initialize(FUnknown* context)
{
  int unit_id = 1;
  if(EditController::initialize(context) != kResultTrue) 
    return kResultFalse;
  for(int m = 0; m < _topo.runtime_modules.size(); m++)
  {
    UnitInfo info;
    info.id = unit_id++;
    info.parentUnitId = kRootUnitId;
    info.programListId = kNoProgramListId;
    memset(info.name, 0, sizeof(info.name));
    auto const& rt_mod = _topo.runtime_modules[m];
    copy_to_vst_string(info.name, 127, rt_mod.name.c_str());

    for (int p = 0; p < rt_mod.params.size(); p++)
    {
      ParameterInfo info;
      auto const& param = rt_mod.params[p].static_topo;
      copy_to_vst_string(info.units, 128, param->unit.c_str());
      info.defaultNormalizedValue = normalize(*param, param->default_value());
    }
  }

  return kResultTrue;
}

}