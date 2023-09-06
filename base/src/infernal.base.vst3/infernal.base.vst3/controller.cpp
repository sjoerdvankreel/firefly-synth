#include <infernal.base/desc.hpp>
#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <infernal.base.vst3/controller.hpp>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace infernal::base::vst3 {

class param_wrapper:
public Parameter
{
  param_topo const* const _topo;
public:
  void toString(ParamValue normalized, String128 string) const override;
  bool fromString(TChar const* string, ParamValue& normalized) const override;

  ParamValue toNormalized(ParamValue plain) const override 
  { return param_value::from_plain(*_topo, plain).to_normalized(*_topo); }
  ParamValue toPlain(ParamValue normalized) const override 
  { return param_value::from_normalized(*_topo, normalized).to_plain(*_topo); }
  param_wrapper(param_topo const* topo, ParameterInfo const& info) : Parameter(info), _topo(topo) {}
};

void 
param_wrapper::toString(ParamValue normalized, String128 string) const
{
  param_value value(param_value::from_normalized(*_topo, normalized));
  from_8bit_string(string, sizeof(String128), value.to_text(*_topo).c_str());
}

bool 
param_wrapper::fromString(TChar const* string, ParamValue& normalized) const
{
  param_value value;
  std::string text(to_8bit_string(string));
  if(!param_value::from_text(*_topo, text, value)) return false;
  normalized = value.to_normalized(*_topo);
  return true;
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
    from_8bit_string(unit_info.name, module.name.c_str());
    addUnit(new Unit(unit_info));

    for (int p = 0; p < module.params.size(); p++)
    {
      ParameterInfo param_info = {};
      auto const& param = module.params[p];
      param_info.id = param.id_hash;
      param_info.unitId = unit_info.id;
      from_8bit_string(param_info.units, param.topo->unit.c_str());
      from_8bit_string(param_info.title, param.topo->name.c_str());
      from_8bit_string(param_info.shortTitle, param.topo->name.c_str());
      param_info.defaultNormalizedValue = param_value::default_value(*param.topo).to_normalized(*param.topo);

      param_info.flags = ParameterInfo::kNoFlags;
      if(param.topo->direction == param_direction::input)
        param_info.flags |= ParameterInfo::kCanAutomate;
      else
        param_info.flags |= ParameterInfo::kIsReadOnly;
      if(param.topo->display == param_display::list)
        param_info.flags |= ParameterInfo::kIsList;
      param_info.stepCount = 0;
      if (!param.topo->is_real())
        param_info.stepCount = param.topo->max - param.topo->min;

      parameters.addParameter(new param_wrapper(module.params[p].topo, param_info));
    }
  }

  return kResultTrue;
}

}