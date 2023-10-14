#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/shared/value.hpp>

#include <plugin_base.vst3/utility.hpp>
#include <plugin_base.vst3/inf_editor.hpp>
#include <plugin_base.vst3/inf_controller.hpp>

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
  ParamValue toNormalized(ParamValue plain) const override 
  { return _topo->domain.raw_to_normalized(plain).value(); }
  ParamValue toPlain(ParamValue normalized) const override 
  { return _topo->domain.normalized_to_raw(normalized_value(normalized)); }

  void toString(ParamValue normalized, String128 string) const override;
  bool fromString(TChar const* string, ParamValue& normalized) const override;
  param_wrapper(param_topo const* topo, ParameterInfo const& info) : Parameter(info), _topo(topo) {}
};

void 
param_wrapper::toString(ParamValue normalized, String128 string) const
{
  auto text = _topo->domain.normalized_to_text(normalized_value(normalized));
  from_8bit_string(string, sizeof(String128) / sizeof(string[0]), text.c_str());
}

bool 
param_wrapper::fromString(TChar const* string, ParamValue& normalized) const
{
  normalized_value base_normalized;
  std::string text(to_8bit_string(string));
  if(!_topo->domain.text_to_normalized(text, base_normalized)) return false;
  normalized = base_normalized.value();
  return true;
}

IPlugView* PLUGIN_API 
inf_controller::createView(char const* name)
{
  if (ConstString(name) != ViewType::kEditor) return nullptr;
  return _editor = new inf_editor(this);
}

void
inf_controller::gui_changing(int index, plain_value plain)
{
  int tag = gui_state().desc().mappings.index_to_tag[index];
  auto normalized = gui_state().desc().plain_to_normalized_at_index(index, plain).value();

  // Per-the-spec we should not have to call setParamNormalized here but not all hosts agree.
  performEdit(tag, normalized);
  setParamNormalized(tag, normalized);
}

tresult PLUGIN_API 
inf_controller::setParamNormalized(ParamID tag, ParamValue value)
{
  if(EditControllerEx1::setParamNormalized(tag, value) != kResultTrue) 
    return kResultFalse;
  int index = gui_state().desc().mappings.tag_to_index.at(tag);
  _gui_state.set_normalized_at_index(index, normalized_value(value));
  if (_editor == nullptr) return kResultTrue;
  _editor->plugin_changed(index, _gui_state.get_plain_at_index(index));
  return kResultTrue;
}

tresult PLUGIN_API
inf_controller::setComponentState(IBStream* state)
{
  if (!load_state(state, gui_state()))
    return kResultFalse;
  for (int p = 0; p < gui_state().desc().param_count; p++)
    gui_changed(p, gui_state().get_plain_at_index(p));
  return kResultOk;
}

tresult PLUGIN_API 
inf_controller::initialize(FUnknown* context)
{
  int unit_id = 1;
  if(EditController::initialize(context) != kResultTrue) 
    return kResultFalse;

  for(int m = 0; m < gui_state().desc().modules.size(); m++)
  {
    auto const& module = gui_state().desc().modules[m];
    UnitInfo unit_info;
    unit_info.id = unit_id++;
    unit_info.parentUnitId = kRootUnitId;
    unit_info.programListId = kNoProgramListId;
    from_8bit_string(unit_info.name, module.info.name.c_str());
    addUnit(new Unit(unit_info));

    for (int p = 0; p < module.params.size(); p++)
    {
      ParameterInfo param_info = {};
      auto const& param = module.params[p];
      param_info.unitId = unit_info.id;
      param_info.id = param.info.id_hash;
      from_8bit_string(param_info.title, param.full_name.c_str());
      from_8bit_string(param_info.shortTitle, param.full_name.c_str());
      from_8bit_string(param_info.units, param.param->domain.unit.c_str());
      param_info.defaultNormalizedValue = param.param->domain.default_normalized().value();

      param_info.flags = ParameterInfo::kNoFlags;
      if(param.param->dsp.direction == param_direction::input)
        param_info.flags |= ParameterInfo::kCanAutomate;
      else
        param_info.flags |= ParameterInfo::kIsReadOnly;
      if(param.param->gui.edit_type == gui_edit_type::list)
        param_info.flags |= ParameterInfo::kIsList;
      param_info.stepCount = 0;
      if (!param.param->domain.is_real())
        param_info.stepCount = param.param->domain.max - param.param->domain.min;
      parameters.addParameter(new param_wrapper(module.params[p].param, param_info));
    }
  }

  return kResultTrue;
}

}