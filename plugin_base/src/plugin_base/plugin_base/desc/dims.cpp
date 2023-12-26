#include <plugin_base/desc/dims.hpp>

namespace plugin_base {

plugin_dims::
plugin_dims(plugin_topo const& plugin, int polyphony)
{
  for (int v = 0; v < polyphony; v++)
  {
    voice_module_slot.emplace_back();
    for (int m = 0; m < plugin.modules.size(); m++)
    {
      auto const& module = plugin.modules[m];
      voice_module_slot[v].push_back(module.info.slot_count);
    }
  }

  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    module_slot.push_back(module.info.slot_count);
    module_slot_midi.emplace_back();
    module_slot_param_slot.emplace_back();
    for(int mi = 0; mi < module.info.slot_count; mi++)
    {
      module_slot_param_slot[m].emplace_back();
      module_slot_midi[m].emplace_back(module.midi_sources.size());
      for (int p = 0; p < module.params.size(); p++)
        module_slot_param_slot[m][mi].push_back(module.params[p].info.slot_count);
    }
  }

  validate(plugin, polyphony);
}

void
plugin_dims::validate(plugin_topo const& plugin, int polyphony) const
{
  assert(voice_module_slot.size() == polyphony);
  for (int v = 0; v < polyphony; v++)
  {
    assert(voice_module_slot[v].size() == plugin.modules.size());
    for (int m = 0; m < plugin.modules.size(); m++)
      assert(voice_module_slot[v][m] == plugin.modules[m].info.slot_count);
  }

  assert(module_slot.size() == plugin.modules.size());
  assert(module_slot_param_slot.size() == plugin.modules.size());
  for (int m = 0; m < plugin.modules.size(); m++)
  {
    auto const& module = plugin.modules[m];
    assert(module_slot[m] == module.info.slot_count);
    assert(module_slot_param_slot[m].size() == module.info.slot_count);
    for (int mi = 0; mi < module.info.slot_count; mi++)
    {
      assert(module_slot_param_slot[m][mi].size() == module.params.size());
      for (int p = 0; p < module.params.size(); p++)
        assert(module_slot_param_slot[m][mi][p] == module.params[p].info.slot_count);
    }
  }
}

}
