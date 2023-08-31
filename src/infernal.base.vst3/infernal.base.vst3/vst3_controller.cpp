#include <infernal.base.vst3/vst3_support.hpp>
#include <infernal.base.vst3/vst3_controller.hpp>

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
    auto const& rt_mod = _topo.runtime_modules[m];
    UnitInfo unit_info;
    unit_info.id = unit_id++;
    unit_info.parentUnitId = kRootUnitId;
    unit_info.programListId = kNoProgramListId;
    to_vst_string(unit_info.name, 128, rt_mod.name.c_str());
    addUnit(new Unit(unit_info));

    for (int p = 0; p < rt_mod.params.size(); p++)
    {
      ParameterInfo param_info;
      auto const& rt_param = rt_mod.params[p];
      auto const& static_param = rt_mod.params[p].static_topo;
      to_vst_string(param_info.units, 128, static_param->unit.c_str());
      to_vst_string(param_info.title, 128, static_param->name.c_str());
      to_vst_string(param_info.shortTitle, 128, static_param->name.c_str());
      param_info.unitId = unit_info.id;
      param_info.id = rt_param.id_hash;
      param_info.defaultNormalizedValue = normalize(*static_param, static_param->default_value());

      param_info.flags = ParameterInfo::kNoFlags;
      if(static_param->direction == param_direction::input) 
        param_info.flags |= ParameterInfo::kCanAutomate;
      else
        param_info.flags |= ParameterInfo::kIsReadOnly;
      if(static_param->display == param_display::list)
        param_info.flags |= ParameterInfo::kIsList;
      param_info.stepCount = 0;
      if (static_param->format == param_format::step)
        param_info.stepCount = static_param->max - static_param->min + 1;

      parameters.addParameter(param_info);
    }
  }

  return kResultTrue;
}

}