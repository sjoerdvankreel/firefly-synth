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
  root_json->setProperty("plugin_id", var(String(topo.id)));
  root_json->setProperty("plugin_name", var(String(topo.name)));
  root_json->setProperty("plugin_type", var((int)topo.type));
  root_json->setProperty("plugin_version_major", var(topo.version_major));
  root_json->setProperty("plugin_version_minor", var(topo.version_minor));
  root_json->setProperty("plugin_module_count", var((int)topo.modules.size()));

  var module_topos_json;
  for (int m = 0; m < topo.modules.size(); m++)
  {
    auto const& module_topo = topo.modules[m];
    auto module_topo_json = std::make_unique<DynamicObject>();
    module_topo_json->setProperty("module_id", var(String(module_topo.id)));
    module_topo_json->setProperty("module_name", var(String(module_topo.name)));
    module_topo_json->setProperty("module_scope", var((int)module_topo.scope));
    module_topo_json->setProperty("module_output", var((int)module_topo.output));
    module_topo_json->setProperty("module_param_count", var((int)module_topo.params.size()));

    var param_topos_json;
    for (int p = 0; p < module_topo.params.size(); p++)
    {
      auto const& param_topo = module_topo.params[p];
      auto param_topo_json = std::make_unique<DynamicObject>();
      param_topo_json->setProperty("param_id", var(param_topo.id));
      param_topo_json->setProperty("param_name", var(param_topo.name));
      param_topo_json->setProperty("param_rate", var((int)param_topo.rate));
      param_topo_json->setProperty("param_type", var((int)param_topo.type));
      param_topo_json->setProperty("param_instance_count", var(param_topo.count));
      param_topo_json->setProperty("param_direction", var((int)param_topo.direction));
      param_topos_json.append(var(param_topo_json.release()));
    }
    module_topo_json->setProperty("module_params", param_topos_json);

    var module_instances_json;
    for (int mi = 0; mi < module_topo.count; mi++)
    {
      var module_instance_json;
      for (int p = 0; p < module_topo.params.size(); p++)
      {
        auto const& param_topo = module_topo.params[p];
        if(param_topo.direction == param_direction::output) continue;
        var param_instances;
        auto param_json = std::make_unique<DynamicObject>();
        param_json->setProperty("param_id", String(param_topo.id));
        for (int pi = 0; pi < param_topo.count; pi++)
          switch (param_topo.type)
          {
          case param_type::log:
          case param_type::step:
          case param_type::name:
          case param_type::linear:
            param_instances.append(var(String(param_topo.plain_to_text(state[m][mi][p][pi]))));
            break;
          case param_type::item:
            param_instances.append(var(String(param_topo.items[state[m][mi][p][pi].step()].id)));
            break;
          default:
            assert(false);
          }
        param_json->setProperty("param_instances", param_instances);
        module_instance_json.append(var(param_json.release()));
      }
      module_instances_json.append(module_instance_json);
    }
    module_topo_json->setProperty("module_instances", module_instances_json);
    module_topos_json.append(var(module_topo_json.release()));
  }

  root_json->setProperty("plugin_modules", module_topos_json);
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