#include <plugin_base/gui/components.hpp>

using namespace juce;

namespace plugin_base {

binding_component::
binding_component(
  plugin_gui* gui, module_desc const* module, 
  gui_bindings const* bindings, int own_slot_index):
_gui(gui), _own_slot_index(own_slot_index), _bindings(bindings), _module(module)
{
  setup_bindings(bindings->enabled.params, _enabled_params);
  setup_bindings(bindings->visible.params, _visibility_params);
}

binding_component::
~binding_component()
{
  for(int i = 0; i < _visibility_params.size(); i++)
    _gui->remove_plugin_listener(_visibility_params[i], this);
  for (int i = 0; i < _enabled_params.size(); i++)
    _gui->remove_plugin_listener(_enabled_params[i], this);
}

bool 
binding_component::bind(
  gui_binding const& binding, 
  std::vector<int> const& params, std::vector<int>& values)
{
  values.clear();
  for (int i = 0; i < params.size(); i++)
  {
    auto const& mapping = _gui->desc()->mappings.params[params[i]];
    values.push_back(mapping.value_at(_gui->gui_state()).step());
  }
  return binding.selector(values);
}

void
binding_component::init()
{
  // Must be called by subclass constructor as we dynamic_cast to Component inside.
  if (_enabled_params.size() != 0)
  {
    auto const& enabled_mapping = _gui->desc()->mappings.params[_enabled_params[0]];
    plugin_changed(_enabled_params[0], enabled_mapping.value_at(_gui->gui_state()));
  }
  if (_visibility_params.size() != 0)
  {
    auto const& visibility_mapping = _gui->desc()->mappings.params[_visibility_params[0]];
    plugin_changed(_visibility_params[0], visibility_mapping.value_at(_gui->gui_state()));
  }
}

void 
binding_component::setup_bindings(
  std::vector<int> const& topo_params, std::vector<int>& params)
{
  for (int i = 0; i < topo_params.size(); i++)
  {
    auto const& param_topo_to_index = _gui->desc()->mappings.topo_to_index;
    auto const& slots = param_topo_to_index[_module->info.topo][_module->info.slot][topo_params[i]];
    bool single_slot = _module->module->params[topo_params[i]].info.slot_count == 1;
    int state_index = single_slot ? slots[0] : slots[_own_slot_index];
    params.push_back(state_index);
    _gui->add_plugin_listener(state_index, this);
  }
}

void
binding_component::plugin_changed(int index, plain_value plain)
{  
  auto& self = dynamic_cast<Component&>(*this);
  auto enabled_iter = std::find(_enabled_params.begin(), _enabled_params.end(), index);
  if (enabled_iter != _enabled_params.end())
    self.setEnabled(bind(_bindings->enabled, _enabled_params, _enabled_values));

  auto visibility_iter = std::find(_visibility_params.begin(), _visibility_params.end(), index);
  if (visibility_iter != _visibility_params.end())
  {
    bool visible = bind(_bindings->visible, _visibility_params, _visibility_values);
    self.setVisible(visible);
    self.setInterceptsMouseClicks(visible, visible);
  }
}

}