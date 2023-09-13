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
  auto root_json = std::make_unique<DynamicObject>();
  root_json->setProperty("id", var(String(topo.id)));
  root_json->setProperty("name", var(String(topo.name)));
  root_json->setProperty("type", var((int)topo.type));
  root_json->setProperty("version_major", var(topo.version_major));
  root_json->setProperty("version_minor", var(topo.version_minor));
  root_json->setProperty("module_count", var((int)topo.modules.size()));

  var module_topos_json;
  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module_topo = topo.modules[m];
    auto module_topo_json = std::make_unique<DynamicObject>();
    module_topo_json->setProperty("count", var(module_topo.count));
    module_topo_json->setProperty("id", var(String(module_topo.id)));
    module_topo_json->setProperty("name", var(String(module_topo.name)));
    module_topo_json->setProperty("scope", var((int)module_topo.scope));
    module_topo_json->setProperty("output", var((int)module_topo.output));
    module_topo_json->setProperty("param_count", var((int)module_topo.params.size()));

    var param_topos_json;
    for (int p = 0; p < module_topo.params.size(); p++)
    {
      auto const& param_topo = module_topo.params[p];
      auto param_topo_json = std::make_unique<DynamicObject>();
      param_topo_json->setProperty("id", var(param_topo.id));
      param_topo_json->setProperty("name", var(param_topo.name));
      param_topo_json->setProperty("count", var(param_topo.count));
      param_topo_json->setProperty("rate", var((int)param_topo.rate));
      param_topo_json->setProperty("type", var((int)param_topo.type));
      param_topo_json->setProperty("direction", var((int)param_topo.direction));
      param_topos_json.append(var(param_topo_json.release()));
    }
    module_topo_json->setProperty("params", param_topos_json);

    var module_instances_json;
    for (int mi = 0; mi < module_topo.count; mi++)
    {
      auto module_instance_json = std::make_unique<DynamicObject>();
      for (int p = 0; p < module_topo.params.size(); p++)
      {
        auto const& param_topo = module_topo.params[p];        
        module_instance_json->setProperty(String(param_topo.id), var(123));
      }
      module_instances_json.append(var(module_instance_json.release()));
    }
    module_topo_json->setProperty("instances", module_instances_json);
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