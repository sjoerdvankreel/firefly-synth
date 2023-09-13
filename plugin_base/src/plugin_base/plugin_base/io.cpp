#include <plugin_base/io.hpp>
#include <juce_core/juce_core.h>
#include <memory>
#include <fstream>

using namespace juce;

namespace plugin_base {

bool 
io_load_file(
  plugin_topo const& topo, 
  std::filesystem::path const& path, 
  jarray4d<plain_value>& state)
{
  return {};
}

bool  
io_store_file(
  plugin_topo const& topo, 
  jarray4d<plain_value> const& state, 
  std::filesystem::path const& path)
{
  // really is binary -- we handled all the cr/lf already, don't duplicate
  std::ofstream stream(path, std::ios::out | std::ios::binary);
  if(stream.bad()) return false;
  auto data(io_store(topo, state));
  stream.write(data.data(), data.size());
  return !stream.bad();
}

std::vector<char>
io_store(
  plugin_topo const& topo, 
  jarray4d<plain_value> const& state)
{
  var module_topos_json;
  auto root_json = std::make_unique<DynamicObject>();
  root_json->setProperty("id", var(String(topo.id)));
  root_json->setProperty("name", var(String(topo.name)));
  root_json->setProperty("type", var((int)topo.type));
  root_json->setProperty("version_major", var(topo.version_major));
  root_json->setProperty("version_minor", var(topo.version_minor));
  root_json->setProperty("module_count", var((int)topo.modules.size()));
  for (int m = 0; m < topo.modules.size(); m++)
  {
    var modules_json;
    auto const& module_topo = topo.modules[m];
    auto module_topo_json = std::make_unique<DynamicObject>();
    module_topo_json->setProperty("count", var(module_topo.count));
    module_topo_json->setProperty("id", var(String(module_topo.id)));
    module_topo_json->setProperty("name", var(String(module_topo.name)));
    module_topo_json->setProperty("scope", var((int)module_topo.scope));
    module_topo_json->setProperty("output", var((int)module_topo.output));
    module_topo_json->setProperty("param_count", var((int)module_topo.params.size()));
    for (int mi = 0; mi < module_topo.count; mi++)
    {
      auto module_json = std::make_unique<DynamicObject>();
      module_json->setProperty("INDEX", mi);
      modules_json.append(var(module_json.release()));
    }
    module_topo_json->setProperty("instances", modules_json);
    module_topos_json.append(var(module_topo_json.release()));
  }
  root_json->setProperty("modules", module_topos_json);
  std::string json = JSON::toString(var(root_json.release())).toStdString();
  return std::vector<char>(json.begin(), json.end());
}


bool 
io_load(
  plugin_topo const& topo, 
  std::vector<char> const& blob,
  jarray4d<plain_value>& state)
{
  return {};
}

}