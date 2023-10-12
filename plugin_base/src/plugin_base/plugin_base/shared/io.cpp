#include <plugin_base/shared/io.hpp>

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>

#include <memory>
#include <fstream>

using namespace juce;

namespace plugin_base {

// file format
static int const version = 1;
static std::string const magic = "{296BBDE2-6411-4A85-BFAF-A9A7B9703DF0}";

load_result
plugin_io_load_file(
  std::filesystem::path const& path, plugin_state& state)
{
  load_result failed("Could not read file.");
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if(stream.bad()) return failed;
  std::streamsize size = stream.tellg();
  if(size <= 0) return failed;
  stream.seekg(0, std::ios::beg);
  std::vector<char> data(size, 0);
  stream.read(data.data(), size);
  if (stream.bad()) return failed;
  return plugin_io_load(data, state);
};

bool
plugin_io_save_file(
  std::filesystem::path const& path, plugin_state const& state)
{
  std::ofstream stream(path, std::ios::out | std::ios::binary);
  if (stream.bad()) return false;
  auto data(plugin_io_save(state));
  stream.write(data.data(), data.size());
  return !stream.bad();
}

std::vector<char>
plugin_io_save(plugin_state const& state)
{
  auto root = std::make_unique<DynamicObject>();
  root->setProperty("magic", var(magic));
  root->setProperty("version", var(version));

  auto plugin = std::make_unique<DynamicObject>();
  plugin->setProperty("id", String(state.desc().plugin->tag.id));
  plugin->setProperty("name", String(state.desc().plugin->tag.name));
  plugin->setProperty("version_major", state.desc().plugin->version_major);
  plugin->setProperty("version_minor", state.desc().plugin->version_minor);
  
  // store some topo info so we can provide meaningful warnings
  var modules;
  for (int m = 0; m < state.desc().plugin->modules.size(); m++)
  {
    var params;
    auto const& module_topo = state.desc().plugin->modules[m];
    auto module = std::make_unique<DynamicObject>();
    module->setProperty("id", String(module_topo.info.tag.id));
    module->setProperty("name", String(module_topo.info.tag.name));
    module->setProperty("slot_count", module_topo.info.slot_count);
    for (int p = 0; p < module_topo.params.size(); p++)
    {
      auto const& param_topo = module_topo.params[p];
      if(param_topo.dsp.direction == param_direction::output) continue;
      auto param = std::make_unique<DynamicObject>();
      param->setProperty("id", String(param_topo.info.tag.id));
      param->setProperty("name", String(param_topo.info.tag.name));
      param->setProperty("slot_count", param_topo.info.slot_count);
      params.append(var(param.release()));
    }
    module->setProperty("params", params);
    modules.append(var(module.release()));
  }  
  plugin->setProperty("modules", modules); 

  // dump the textual values in 4d format
  var module_states;
  for (int m = 0; m < state.desc().plugin->modules.size(); m++)
  {
    var module_slot_states;
    auto const& module_topo = state.desc().plugin->modules[m];
    auto module_state = std::make_unique<DynamicObject>();
    for (int mi = 0; mi < module_topo.info.slot_count; mi++)
    {
      var param_states;
      auto module_slot_state = std::make_unique<DynamicObject>();
      for (int p = 0; p < module_topo.params.size(); p++)
      {
        var param_slot_states;
        auto const& param_topo = module_topo.params[p];
        if(param_topo.dsp.direction == param_direction::output) continue;
        auto param_state = std::make_unique<DynamicObject>();
        for (int pi = 0; pi < param_topo.info.slot_count; pi++)
          param_slot_states.append(var(String(param_topo.domain.plain_to_text(state.state()[m][mi][p][pi]))));
        param_state->setProperty("slots", param_slot_states);
        param_states.append(var(param_state.release()));
      }
      module_slot_state->setProperty("params", param_states);
      module_slot_states.append(var(module_slot_state.release()));
    }
    module_state->setProperty("slots", module_slot_states);
    module_states.append(var(module_state.release()));
  }  
  plugin->setProperty("state", module_states);

  // checksum so on load we can assume integrity of the plugin block 
  // instead of doing lots of structural json validation
  var plugin_var(plugin.release());
  String plugin_text(JSON::toString(plugin_var));
  root->setProperty("checksum", var(MD5(plugin_text.toUTF8()).toHexString()));

  root->setProperty("plugin", plugin_var);
  std::string json = JSON::toString(var(root.release())).toStdString();
  return std::vector<char>(json.begin(), json.end());
}

load_result
plugin_io_load(
  std::vector<char> const& data, plugin_state& state)
{
  var root;
  std::string json(data.size(), '\0');
  std::copy(data.begin(), data.end(), json.begin());
  auto parse_result = JSON::parse(String(json), root);
  if(!parse_result.wasOk()) 
    return load_result("Invalid json.");
  if(!root.hasProperty("plugin")) 
    return load_result("Invalid plugin.");
  if(!root.hasProperty("checksum")) 
    return load_result("Invalid checksum.");
  if(!root.hasProperty("magic") || root["magic"] != magic) 
    return load_result("Invalid magic.");
  if(!root.hasProperty("version") || (int)root["version"] > version) 
    return load_result("Invalid version.");

  var plugin = root["plugin"];
  if(plugin["id"] != state.desc().plugin->tag.id)
    return load_result("Invalid plugin id.");
  if((int)plugin["version_major"] > state.desc().plugin->version_major)
    return load_result("Invalid plugin version.");
  if((int)plugin["version_major"] == state.desc().plugin->version_major)
    if((int)plugin["version_minor"] > state.desc().plugin->version_minor)
      return load_result("Invalid plugin version.");
  if(root["checksum"] != MD5(JSON::toString(plugin).toUTF8()).toHexString()) 
    return load_result("Invalid checksum.");

  // good to go - only warnings from now on
  load_result result;
  state.init_defaults();
  for(int m = 0; m < plugin["modules"].size(); m++)
  {
    // check for old module not found
    auto module_id = plugin["modules"][m]["id"].toString().toStdString();
    auto module_name = plugin["modules"][m]["name"].toString().toStdString();
    auto module_iter = state.desc().module_id_to_index.find(module_id);
    if (module_iter == state.desc().module_id_to_index.end())
    {
      result.warnings.push_back("Module '" + module_name + "' was deleted.");
      continue;
    }

    // check for changed module slot count
    var module_slot_count = plugin["modules"][m]["slot_count"];
    auto const& new_module = state.desc().plugin->modules[module_iter->second];
    if ((int)module_slot_count != new_module.info.slot_count)
      result.warnings.push_back("Module '" + new_module.info.tag.name + "' changed slot count.");

    for (int p = 0; p < plugin["modules"][m]["params"].size(); p++)
    {
      // check for old param not found
      auto param_id = plugin["modules"][m]["params"][p]["id"].toString().toStdString();
      auto param_name = plugin["modules"][m]["params"][p]["name"].toString().toStdString();
      auto param_iter = state.desc().mappings.id_to_index.at(module_id).find(param_id);
      if (param_iter == state.desc().mappings.id_to_index.at(module_id).end())
      {
        result.warnings.push_back("Param '" + module_name + " " + param_name + "' was deleted.");
        continue;
      }

      // check for changed param slot count
      var param_slot_count = plugin["modules"][m]["params"][p]["slot_count"];
      auto const& new_param = state.desc().plugin->modules[module_iter->second].params[param_iter->second];
      if ((int)param_slot_count != new_param.info.slot_count)
        result.warnings.push_back("Param '" + new_module.info.tag.name + " " + new_param.info.tag.name + "' slot count changed.");
    }
  }

  // copy over old state, push parse warnings as we go
  for (int m = 0; m < plugin["state"].size(); m++)
  {
    auto module_id = plugin["modules"][m]["id"].toString().toStdString();
    auto module_iter = state.desc().module_id_to_index.find(module_id);
    if(module_iter == state.desc().module_id_to_index.end()) continue;
    var module_slots = plugin["state"][m]["slots"];
    auto const& new_module = state.desc().plugin->modules[module_iter->second];

    for(int mi = 0; mi < module_slots.size() && mi < new_module.info.slot_count; mi++)
      for (int p = 0; p < module_slots[mi]["params"].size(); p++)
      {
        auto param_id = plugin["modules"][m]["params"][p]["id"].toString().toStdString();
        auto param_iter = state.desc().mappings.id_to_index.at(module_id).find(param_id);
        if (param_iter == state.desc().mappings.id_to_index.at(module_id).end()) continue;
        var param_slots = plugin["state"][m]["slots"][mi]["params"][p]["slots"];
        auto const& new_param = state.desc().plugin->modules[module_iter->second].params[param_iter->second];
        for (int pi = 0; pi < param_slots.size() && pi < new_param.info.slot_count; pi++)
        {
          plain_value plain;
          auto const& topo = state.desc().plugin->modules[module_iter->second].params[param_iter->second];
          std::string text = plugin["state"][m]["slots"][mi]["params"][p]["slots"][pi].toString().toStdString();
          if(topo.domain.text_to_plain(text, plain))
            state.state()[module_iter->second][mi][param_iter->second][pi] = plain;
          else
            result.warnings.push_back("Param '" + new_module.info.tag.name + " " + new_param.info.tag.name + "': invalid value '" + text + "'.");
        }
      }
  }

  return result;
}

}