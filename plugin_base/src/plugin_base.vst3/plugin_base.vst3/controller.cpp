#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base.vst3/editor.hpp>
#include <plugin_base.vst3/controller.hpp>
#include <base/source/fstring.h>
#include <juce_events/juce_events.h>

using namespace juce;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

class param_wrapper:
public Parameter
{
  param_topo const* const _topo;
public:
  void toString(ParamValue normalized, String128 string) const override;
  bool fromString(TChar const* string, ParamValue& normalized) const override;

  ParamValue toNormalized(ParamValue plain) const override 
  { return _topo->raw_to_normalized(plain).value(); }
  ParamValue toPlain(ParamValue normalized) const override 
  { return _topo->normalized_to_raw(normalized_value(normalized)); }
  param_wrapper(param_topo const* topo, ParameterInfo const& info) : Parameter(info), _topo(topo) {}
};

void 
param_wrapper::toString(ParamValue normalized, String128 string) const
{
  plain_value plain(_topo->normalized_to_plain(normalized_value(normalized)));
  from_8bit_string(string, sizeof(String128) / sizeof(string[0]), _topo->plain_to_text(plain).c_str());
}

bool 
param_wrapper::fromString(TChar const* string, ParamValue& normalized) const
{
  plain_value plain;
  std::string text(to_8bit_string(string));
  if(!_topo->text_to_plain(text, plain)) return false;
  normalized = _topo->plain_to_normalized(plain).value();
  return true;
}

IPlugView* PLUGIN_API 
controller::createView(char const* name)
{
  if (ConstString(name) != ViewType::kEditor) return nullptr;
  MessageManager::getInstance();
  return _editor = new editor(this, _topo_factory);
}

tresult PLUGIN_API 
controller::setParamNormalized(ParamID tag, ParamValue value)
{
  if(EditControllerEx1::setParamNormalized(tag, value) != kResultTrue) 
    return kResultFalse;
  if(_editor == nullptr) return kResultTrue;
  int param_index = _desc.id_to_index.at(tag);
  param_mapping mapping = _desc.param_mappings[param_index];
  plain_value plain = _desc.param_at(mapping).topo->normalized_to_plain(normalized_value(value));
  _editor->plugin_param_changed(param_index, plain);
  return kResultTrue;
}

tresult PLUGIN_API 
controller::initialize(FUnknown* context)
{
  int unit_id = 1;
  if(EditController::initialize(context) != kResultTrue) 
    return kResultFalse;

  for(int m = 0; m < _desc.modules.size(); m++)
  {
    auto const& module = _desc.modules[m];
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
      param_info.defaultNormalizedValue = param.topo->default_normalized().value();

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