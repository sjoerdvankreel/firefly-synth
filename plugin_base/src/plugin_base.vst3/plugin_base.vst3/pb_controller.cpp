#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base/shared/logger.hpp>

#include <plugin_base.vst3/utility.hpp>
#include <plugin_base.vst3/pb_param.hpp>
#include <plugin_base.vst3/pb_editor.hpp>
#include <plugin_base.vst3/pb_controller.hpp>

#include <base/source/fstring.h>
#include <pluginterfaces/vst/ivstcontextmenu.h>

#include <juce_events/juce_events.h>
#include <stack>

using namespace juce;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace plugin_base::vst3 {

pb_basic_config const* 
pb_basic_config::instance()
{
  static pb_basic_config result = {};
  return &result;
}

pb_controller::
~pb_controller() 
{ _automation_state.remove_any_listener(this); }

pb_controller::
pb_controller(plugin_topo const* topo):
_desc(std::make_unique<plugin_desc>(topo, this)),
_automation_state(_desc.get(), true),
_extra_state(gui_extra_state_keyset(*_desc->plugin))
{ 
  PB_LOG_FUNC_ENTRY_EXIT();
  _automation_state.add_any_listener(this);
}

void 
pb_controller::gui_param_begin_changes(int index) 
{ 
  _undo_tokens.push(_automation_state.begin_undo_region());
  beginEdit(automation_state().desc().param_mappings.index_to_tag[index]);
}

void
pb_controller::gui_param_end_changes(int index)
{
  endEdit(automation_state().desc().param_mappings.index_to_tag[index]);
  automation_state().end_undo_region(_undo_tokens.top(), "Change", automation_state().desc().params[index]->full_name);
  _undo_tokens.pop();
}

IPlugView* PLUGIN_API
pb_controller::createView(char const* name)
{
  PB_LOG_FUNC_ENTRY_EXIT();
  if (ConstString(name) != ViewType::kEditor) return nullptr;
  return _editor = new pb_editor(this, &_modulation_outputs);
}

tresult PLUGIN_API
pb_controller::getState(IBStream* state)
{
  PB_LOG_FUNC_ENTRY_EXIT();
  std::vector<char> data(plugin_io_save_extra_state(*_automation_state.desc().plugin, _extra_state));
  return state->write(data.data(), data.size());
}

tresult PLUGIN_API 
pb_controller::setState(IBStream* state)
{
  PB_LOG_FUNC_ENTRY_EXIT();
  if (!plugin_io_load_extra_state(*_automation_state.desc().plugin, load_ibstream(state), _extra_state).ok())
    return kResultFalse;
  return kResultOk;
}

tresult PLUGIN_API
pb_controller::setComponentState(IBStream* state)
{
  PB_LOG_FUNC_ENTRY_EXIT();
  automation_state().begin_undo_region();
  if (!plugin_io_load_instance_state(load_ibstream(state), automation_state(), false).ok())
  {
    automation_state().discard_undo_region();
    return kResultFalse;
  }
  for (int p = 0; p < automation_state().desc().param_count; p++)
    gui_param_changed(p, automation_state().get_plain_at_index(p));
  automation_state().discard_undo_region();
  return kResultOk;
}

tresult PLUGIN_API 
pb_controller::setParamNormalized(ParamID tag, ParamValue value)
{
  _inside_set_param_normalized = true;
  if(EditControllerEx1::setParamNormalized(tag, value) != kResultTrue) 
  {
    _inside_set_param_normalized = false;
    return kResultFalse;
  }
  
  // fake midi params are not mapped
  auto mapping_iter = automation_state().desc().param_mappings.tag_to_index.find(tag);
  if (mapping_iter != automation_state().desc().param_mappings.tag_to_index.end())
  {
    _automation_state.set_normalized_at_index(mapping_iter->second, normalized_value(value));
    if (_editor) _editor->automation_state_changed(mapping_iter->second, normalized_value(value));
  }

  _inside_set_param_normalized = false;
  return kResultTrue;
}

tresult PLUGIN_API 
pb_controller::getMidiControllerAssignment(int32 bus, int16 channel, CtrlNumber number, ParamID& id)
{
  if(bus != 0) return kResultFalse;
  auto iter = _midi_id_to_param.find(number);
  if(iter == _midi_id_to_param.end()) return kResultFalse;
  id = iter->second;
  return kResultTrue;
}

void
pb_controller::param_state_changed(int index, plain_value plain)
{
  if(_inside_set_param_normalized) return;
  if (_automation_state.desc().params[index]->param->dsp.direction == param_direction::output) return;
  int tag = automation_state().desc().param_mappings.index_to_tag[index];
  auto normalized = automation_state().desc().plain_to_normalized_at_index(index, plain).value();

  // Per-the-spec we should not have to call setParamNormalized here but not all hosts agree.
  performEdit(tag, normalized);
  setParamNormalized(tag, normalized);
}

std::unique_ptr<host_menu>
pb_controller::context_menu(int param_id) const
{
  FUnknownPtr<IComponentHandler3> handler(componentHandler);
  if (handler == nullptr) return {};
  if (_editor == nullptr) return {};

  ParamID id = param_id;
  IContextMenuTarget* target;
  IPtr<IContextMenu> vst_menu(handler->createContextMenu(_editor, &id));
  if(!vst_menu) return {};
  auto result = std::make_unique<host_menu>();
  std::stack<host_menu_item*> menu_stack;
  menu_stack.push(&result->root);

  for (int i = 0; i < vst_menu->getItemCount(); i++)
  {
    IContextMenu::Item vst_item;
    if (vst_menu->getItem(i, vst_item, &target) != kResultOk) return {};
    
    auto item = menu_stack.top()->children.emplace_back(std::make_shared<host_menu_item>());
    item->tag = i;
    item->flags = host_menu_flags_enabled;
    item->name = to_8bit_string(vst_item.name);
    if((vst_item.flags & IContextMenuItem::kIsChecked) == IContextMenuItem::kIsChecked)
      item->flags |= host_menu_flags_checked;
    if ((vst_item.flags & IContextMenuItem::kIsSeparator) == IContextMenuItem::kIsSeparator)
      item->flags |= host_menu_flags_separator;
    
    if ((vst_item.flags & IContextMenuItem::kIsGroupStart) == IContextMenuItem::kIsGroupStart)
      menu_stack.push(item.get());
    else if ((vst_item.flags & IContextMenuItem::kIsGroupEnd) == IContextMenuItem::kIsGroupEnd)
      menu_stack.pop();
    else if ((vst_item.flags & IContextMenuItem::kIsDisabled) == IContextMenuItem::kIsDisabled)
      item->flags &= ~host_menu_flags_enabled;
  }
  assert(menu_stack.size() == 1);
  menu_stack.pop();

  result->clicked = [vst_menu](int tag) { 
    IContextMenu::Item item = {};
    IContextMenuTarget* target = nullptr;
    if(tag < 0) return; // can happen on close
    assert(tag < vst_menu->getItemCount());
    if(vst_menu->getItem(tag, item, &target) != kResultOk) return;
    if(target == nullptr) return;
    target->executeMenuItem(item.tag);
  };
  return result;
}

tresult PLUGIN_API 
pb_controller::initialize(FUnknown* context)
{
  PB_LOG_FUNC_ENTRY_EXIT();

  int unit_id = 1;
  if(EditController::initialize(context) != kResultTrue) 
    return kResultFalse;

  for(int m = 0; m < automation_state().desc().modules.size(); m++)
  {
    auto const& module = automation_state().desc().modules[m];
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
      param_info.defaultNormalizedValue = param.param->domain.default_normalized(module.info.slot, param.info.slot).value();

      param_info.flags = ParameterInfo::kNoFlags;
      if(param.param->dsp.can_automate(module.info.slot))
        param_info.flags |= ParameterInfo::kCanAutomate;
      else
        param_info.flags |= ParameterInfo::kIsReadOnly;
      if(param.param->gui.is_list())
        param_info.flags |= ParameterInfo::kIsList;
      param_info.stepCount = 0;
      if (!param.param->domain.is_real())
        param_info.stepCount = param.param->domain.max - param.param->domain.min;
      parameters.addParameter(new pb_param(&_automation_state, module.params[p].param, module.params[p].info.global, param_info));
    }
  }

  // be sure to append fake midi params *after* the real ones
  // to not mess up the tag to index mapping
  for (int m = 0; m < automation_state().desc().modules.size(); m++)
  {
    auto const& module = automation_state().desc().modules[m];
    for (int ms = 0; ms < module.midi_sources.size(); ms++)
    {
      ParameterInfo param_info = {};
      auto const& source = module.midi_sources[ms];
      param_info.stepCount = 0;
      param_info.unitId = kRootUnitId;
      param_info.id = source.info.id_hash;
      param_info.flags = ParameterInfo::kIsHidden | ParameterInfo::kIsHidden;
      parameters.addParameter(new Parameter(param_info));
      param_info.defaultNormalizedValue = source.source->default_;
      _midi_id_to_param[source.source->id] = param_info.id;
    }
  }

  // make sure no clashes
  std::set<int> hashes = {};
  (void)hashes;
  for (int i = 0; i < parameters.getParameterCount(); i++)
    assert(hashes.insert(parameters.getParameterByIndex(i)->getInfo().id).second);

  return kResultTrue;
}

}