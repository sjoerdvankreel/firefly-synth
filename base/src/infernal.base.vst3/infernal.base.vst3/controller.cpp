#include <infernal.base/desc.hpp>
#include <infernal.base/param_value.hpp>
#include <infernal.base.vst3/controller.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

static void
to_vst_string(TChar* dest, int count, char const* source)
{
  memset(dest, 0, sizeof(*dest) * count);
  for (int i = 0; i < count - 1 && i < strlen(source); i++)
    dest[i] = source[i];
}

tresult PLUGIN_API 
controller::initialize(FUnknown* context)
{
  int unit_id = 1;
  if(EditController::initialize(context) != kResultTrue) 
    return kResultFalse;
  
  plugin_desc desc(_topo);
  for(int m = 0; m < desc.modules.size(); m++)
  {
    auto const& module = desc.modules[m];
    UnitInfo unit_info;
    unit_info.id = unit_id++;
    unit_info.parentUnitId = kRootUnitId;
    unit_info.programListId = kNoProgramListId;
    to_vst_string(unit_info.name, 128, module.name.c_str());
    addUnit(new Unit(unit_info));

    for (int p = 0; p < module.params.size(); p++)
    {
      ParameterInfo param_info;
      auto const& param = module.params[p];
      param_info.id = param.id_hash;
      param_info.unitId = unit_info.id;
      to_vst_string(param_info.units, 128, param.topo->unit.c_str());
      to_vst_string(param_info.title, 128, param.topo->name.c_str());
      to_vst_string(param_info.shortTitle, 128, param.topo->name.c_str());
      param_info.defaultNormalizedValue = param_value::default_value(*param.topo).to_normalized(*param.topo);

      param_info.flags = ParameterInfo::kNoFlags;
      if(param.topo->config.direction == param_direction::input)
        param_info.flags |= ParameterInfo::kCanAutomate;
      else
        param_info.flags |= ParameterInfo::kIsReadOnly;
      if(param.topo->config.display == param_display::list)
        param_info.flags |= ParameterInfo::kIsList;
      param_info.stepCount = 0;
      if(param.topo->config.display == param_display::toggle)
        param_info.stepCount = 1;
      else if (param.topo->config.format == param_format::step)
        param_info.stepCount = param.topo->max - param.topo->min + 1;

      parameters.addParameter(param_info);
    }
  }

  return kResultTrue;
}

}