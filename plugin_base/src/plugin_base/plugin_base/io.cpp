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
  std::ofstream stream(path, std::ios::out);
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
  var modules;
  auto root_json = std::make_unique<DynamicObject>();
  root_json->setProperty("id", var(String(topo.id)));
  root_json->setProperty("name", var(String(topo.name)));
  root_json->setProperty("type", var((int)topo.type));
  root_json->setProperty("version_major", var(topo.version_major));
  root_json->setProperty("version_minor", var(topo.version_minor));
  root_json->setProperty("module_count", var((int)topo.modules.size()));
  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module_topo = topo.modules[m];
    auto module_json = std::make_unique<DynamicObject>();
    module_json->setProperty("count", var(module_topo.count));
    module_json->setProperty("id", var(String(module_topo.id)));
    module_json->setProperty("name", var(String(module_topo.name)));
    module_json->setProperty("scope", var((int)module_topo.scope));
    module_json->setProperty("output", var((int)module_topo.output));
    modules.append(var(module_json.release()));
  }
  root_json->setProperty("modules", modules);
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