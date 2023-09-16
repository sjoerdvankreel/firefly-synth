#include <plugin_base/io.hpp>
#include <juce_core/juce_core.h>

#include <memory>
#include <fstream>

using namespace juce;

namespace plugin_base {

// TODO checksum
// TODO filter out params

static int const format_version = 1;
static std::string const format_magic = "{296BBDE2-6411-4A85-BFAF-A9A7B9703DF0}";

bool
plugin_io::save_file(std::filesystem::path const& path, jarray<plain_value, 4> const& state) const
{
  std::ofstream stream(path, std::ios::out | std::ios::binary);
  if (stream.bad()) return false;
  auto data(save(state));
  stream.write(data.data(), data.size());
  return !stream.bad();
}

bool
plugin_io::load_file(std::filesystem::path const& path, jarray<plain_value, 4>& state) const
{
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if(stream.bad()) return false;
  std::streamsize size = stream.tellg();
  stream.seekg(0, std::ios::beg);
  std::vector<char> data(size, 0);
  stream.read(data.data(), size);
  if (stream.bad()) return false;
  return load(data, state);
}

std::vector<char>
plugin_io::save(jarray<plain_value, 4> const& state) const
{
  auto root = std::make_unique<DynamicObject>();
  
  auto format = std::make_unique<DynamicObject>();
  format->setProperty("magic", var(format_magic));
  format->setProperty("version", var(format_version));
  root->setProperty("format", var(format.release()));

  auto plugin = std::make_unique<DynamicObject>();
  plugin->setProperty("id", String(_topo->id));
  plugin->setProperty("name", String(_topo->name));
  plugin->setProperty("version_major", _topo->version_major);
  plugin->setProperty("version_minor", _topo->version_minor);
  
  // store some topo info so we can provide meaningful warnings
  var modules;
  for (int m = 0; m < _topo->modules.size(); m++)
  {
    var params;
    auto const& module_topo = _topo->modules[m];
    auto module = std::make_unique<DynamicObject>();
    module->setProperty("id", String(module_topo.id));
    module->setProperty("name", String(module_topo.name));
    for (int p = 0; p < module_topo.params.size(); p++)
    {
      auto const& param_topo = module_topo.params[p];
      auto param = std::make_unique<DynamicObject>();
      param->setProperty("id", String(param_topo.id));
      param->setProperty("name", String(param_topo.name));
      params.append(var(param.release()));
    }
    module->setProperty("params", params);
    modules.append(var(module.release()));
  }  
  plugin->setProperty("modules", modules); 
  root->setProperty("plugin", var(plugin.release()));

  // dump the textual values in 4d format
  var module_states;
  for (int m = 0; m < _topo->modules.size(); m++)
  {
    var module_slot_states;
    auto const& module_topo = _topo->modules[m];
    auto module_state = std::make_unique<DynamicObject>();
    for (int mi = 0; mi < module_topo.slot_count; mi++)
    {
      var param_states;
      auto module_slot_state = std::make_unique<DynamicObject>();
      for (int p = 0; p < module_topo.params.size(); p++)
      {
        var param_slot_states;
        auto const& param_topo = module_topo.params[p];
        auto param_state = std::make_unique<DynamicObject>();
        for (int pi = 0; pi < param_topo.slot_count; pi++)
          param_slot_states.append(var(String(param_topo.plain_to_text(state[m][mi][p][pi]))));
        param_state->setProperty("slots", param_slot_states);
        param_states.append(var(param_state.release()));
      }
      module_slot_state->setProperty("params", param_states);
      module_slot_states.append(var(module_slot_state.release()));
    }
    module_state->setProperty("slots", module_slot_states);
    module_states.append(var(module_state.release()));
  }
  root->setProperty("state", module_states);

  std::string json = JSON::toString(var(root.release())).toStdString();
  return std::vector<char>(json.begin(), json.end());
}

bool 
plugin_io::load(std::vector<char> const& data, jarray<plain_value, 4>& state) const
{
  var root;
  std::string json(data.size(), '\0');
  std::copy(data.begin(), data.end(), json.begin());
  auto result = JSON::parse(String(json), root);
  if(!result.wasOk()) return false;
  var format = root["format"];
  if(format["magic"] != String(format_magic)) return false;
  if((int)format["version"] != format_version) return false;
  return true;
}

}