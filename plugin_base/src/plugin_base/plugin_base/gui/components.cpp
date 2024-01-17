#include <plugin_base/gui/components.hpp>

using namespace juce;

namespace plugin_base {

binding_component::
binding_component(
  plugin_gui* gui, module_desc const* module, 
  gui_bindings const* bindings, int own_slot_index):
_gui(gui), _own_slot_index(own_slot_index), _bindings(bindings), _module(module)
{
  setup_param_bindings(bindings->global_enabled, bindings->enabled.params, _enabled_params);
  setup_param_bindings(bindings->global_visible, bindings->visible.params, _visibility_params);
}

binding_component::
~binding_component()
{
  for(int i = 0; i < _visibility_params.size(); i++)
    _gui->gui_state()->remove_listener(_visibility_params[i], this);
  for (int i = 0; i < _enabled_params.size(); i++)
    _gui->gui_state()->remove_listener(_enabled_params[i], this);

  auto const& param_topo_to_index = _gui->gui_state()->desc().param_mappings.topo_to_index;
  auto const& global_enabled = _bindings->global_enabled;
  if (global_enabled.is_bound())
  {
    int global_index = param_topo_to_index[global_enabled.module][0][global_enabled.param][0];
    _gui->gui_state()->remove_listener(global_index, this);
  }
  auto const& global_visible = _bindings->global_visible;
  if (global_visible.is_bound())
  {
    int global_index = param_topo_to_index[global_visible.module][0][global_visible.param][0];
    _gui->gui_state()->remove_listener(global_index, this);
  }
}

bool 
binding_component::bind_param(
  gui_binding const& binding, 
  std::vector<int> const& params, std::vector<int>& values)
{
  values.clear();
  for (int i = 0; i < params.size(); i++)
    values.push_back(_gui->gui_state()->get_plain_at_index(params[i]).step());
  return binding.param_selector(values);
}

void
binding_component::init()
{
  // Must be called by subclass constructor as we dynamic_cast to Component.
  auto& self = dynamic_cast<Component&>(*this);
  self.setVisible(_bindings->visible.slot_selector == nullptr || 
    _bindings->visible.slot_selector(_module->info.slot));
  self.setEnabled(_bindings->enabled.slot_selector == nullptr || 
    _bindings->enabled.slot_selector(_module->info.slot));
  if (_enabled_params.size() != 0)
    state_changed(_enabled_params[0], 
      _gui->gui_state()->get_plain_at_index(_enabled_params[0]));
  if (_visibility_params.size() != 0)
    state_changed(_visibility_params[0],
      _gui->gui_state()->get_plain_at_index(_visibility_params[0]));
}

void
binding_component::setup_param_bindings(
  gui_global_binding const& global_binding,
  std::vector<int> const& topo_params, std::vector<int>& params)
{
  auto const& param_topo_to_index = _gui->gui_state()->desc().param_mappings.topo_to_index;
  for (int i = 0; i < topo_params.size(); i++)
  {
    auto const& slots = param_topo_to_index[_module->info.topo][_module->info.slot][topo_params[i]];
    bool single_slot = _module->module->params[topo_params[i]].info.slot_count == 1;
    int state_index = single_slot ? slots[0] : slots[_own_slot_index];
    params.push_back(state_index);
    _gui->gui_state()->add_listener(state_index, this);
  }
  if (global_binding.is_bound())
  {
    int global_index = param_topo_to_index[global_binding.module][0][global_binding.param][0];
    _gui->gui_state()->add_listener(global_index, this);
  }
}

void
binding_component::state_changed(int index, plain_value plain)
{
  auto& self = dynamic_cast<Component&>(*this);
  if(_bindings->enabled.slot_selector != nullptr && !_bindings->enabled.slot_selector(_module->info.slot))
    self.setEnabled(false);
  else 
  {
    bool global_enabled = true;
    auto const& global_binding = _bindings->global_enabled;
    if(global_binding.is_bound())
    {
      int global_value = _gui->gui_state()->get_plain_at(global_binding.module, 0, global_binding.param, 0).step();
      global_enabled = global_binding.selector(global_value);
    }
    if(!global_enabled)
      self.setEnabled(false);
    else
    {
      auto enabled_iter = std::find(_enabled_params.begin(), _enabled_params.end(), index);
      if (enabled_iter != _enabled_params.end())
        self.setEnabled(bind_param(_bindings->enabled, _enabled_params, _enabled_values));
    }
  }

  if(_bindings->visible.slot_selector != nullptr && !_bindings->visible.slot_selector(_module->info.slot))
    self.setVisible(false);
  else
  {
    bool global_visible = true;
    auto const& global_binding = _bindings->global_visible;
    if(global_binding.is_bound())
    {
      int global_value = _gui->gui_state()->get_plain_at(global_binding.module, 0, global_binding.param, 0).step();
      global_visible = global_binding.selector(global_value);
    }
    if (!global_visible)
    {
      self.setVisible(false);
      self.setInterceptsMouseClicks(false, false);
    }
    else
    {
      auto visibility_iter = std::find(_visibility_params.begin(), _visibility_params.end(), index);
      if (visibility_iter != _visibility_params.end())
      {
        bool visible = bind_param(_bindings->visible, _visibility_params, _visibility_values);
        self.setVisible(visible);
        self.setInterceptsMouseClicks(visible, visible);
      }
    }
  }
}

}