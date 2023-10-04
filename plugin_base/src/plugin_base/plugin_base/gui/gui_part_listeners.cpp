#include <plugin_base/gui/gui.hpp>

namespace plugin_base {

void 
plugin_gui::add_gui_listener(gui_listener* listener) 
{ _gui_listeners.push_back(listener); }
void 
plugin_gui::add_plugin_listener(int index, plugin_listener* listener) 
{ _plugin_listeners[index].push_back(listener); }

void 
plugin_gui::gui_changed(int index, plain_value plain)
{
  if(_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_changed(index, plain);
}

void 
plugin_gui::gui_begin_changes(int index)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for(int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_begin_changes(index);
}

void
plugin_gui::gui_end_changes(int index)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_end_changes(index);
}

void
plugin_gui::gui_changing(int index, plain_value plain)
{
  if (_desc->params[index]->param->dsp.direction == param_direction::input)
    for (int i = 0; i < _gui_listeners.size(); i++)
      _gui_listeners[i]->gui_changing(index, plain);
}

void
plugin_gui::plugin_changed(int index, plain_value plain)
{
  for(int i = 0; i < _plugin_listeners[index].size(); i++)
    _plugin_listeners[index][i]->plugin_changed(index, plain);
}

void 
plugin_gui::remove_gui_listener(gui_listener* listener)
{
  auto iter = std::find(_gui_listeners.begin(), _gui_listeners.end(), listener);
  if(iter != _gui_listeners.end()) _gui_listeners.erase(iter);
}

void 
plugin_gui::remove_plugin_listener(int index, plugin_listener* listener)
{
  auto iter = std::find(_plugin_listeners[index].begin(), _plugin_listeners[index].end(), listener);
  if (iter != _plugin_listeners[index].end()) _plugin_listeners[index].erase(iter);
}

void
plugin_gui::fire_state_loaded()
{
  int param_global = 0;
  for (int m = 0; m < _desc->plugin->modules.size(); m++)
  {
    auto const& module = _desc->plugin->modules[m];
    for (int mi = 0; mi < module.info.slot_count; mi++)
      for (int p = 0; p < module.params.size(); p++)
        for (int pi = 0; pi < module.params[p].info.slot_count; pi++)
          gui_changed(param_global++, (*_gui_state)[m][mi][p][pi]);
  }
}

}