#include <plugin_base/io.hpp>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>

#include <memory>
#include <fstream>

using namespace juce;

namespace plugin_base {

// file format
static int const version = 1;
static std::string const magic = "{296BBDE2-6411-4A85-BFAF-A9A7B9703DF0}";

bool
plugin_io::save_file(std::filesystem::path const& path, jarray<plain_value, 4> const& state) const
{
  std::ofstream stream(path, std::ios::out | std::ios::binary);
  if (stream.bad()) return false;
  auto data(save(state));
  stream.write(data.data(), data.size());
  return !stream.bad();
}

io_result
plugin_io::load_file(std::filesystem::path const& path, jarray<plain_value, 4>& state) const
{
  io_result failed("Could not read file.");
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if(stream.bad()) return failed;
  std::streamsize size = stream.tellg();
  if(size <= 0) return failed;
  stream.seekg(0, std::ios::beg);
  std::vector<char> data(size, 0);
  stream.read(data.data(), size);
  if (stream.bad()) return failed;
  return load(data, state);
}

std::vector<char>
plugin_io::save(jarray<plain_value, 4> const& state) const
{
  auto root = std::make_unique<DynamicObject>();
  root->setProperty("magic", var(magic));
  root->setProperty("version", var(version));

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
      if(param_topo.dir == param_dir::output) continue;
      auto param = std::make_unique<DynamicObject>();
      param->setProperty("id", String(param_topo.id));
      param->setProperty("name", String(param_topo.name));
      params.append(var(param.release()));
    }
    module->setProperty("params", params);
    modules.append(var(module.release()));
  }  
  plugin->setProperty("modules", modules); 

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
        if(param_topo.dir == param_dir::output) continue;
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

io_result 
plugin_io::load(std::vector<char> const& data, jarray<plain_value, 4>& state) const
{
  var root;
  std::string json(data.size(), '\0');
  std::copy(data.begin(), data.end(), json.begin());
  auto parse_result = JSON::parse(String(json), root);
  if(!parse_result.wasOk()) return io_result("Invalid json.");
  if(!root.hasProperty("plugin")) return io_result("Invalid plugin.");
  if(!root.hasProperty("checksum")) return io_result("Invalid checksum.");
  if(!root.hasProperty("magic") || root["magic"] != magic) return io_result("Invalid magic.");
  if(!root.hasProperty("version") || (int)root["version"] > version) return io_result("Invalid version.");  

  io_result result;
  var plugin = root["plugin"];
  if(plugin["id"] != _topo->id) return io_result("Invalid plugin id.");


  // TODO if(root["checksum"] != MD5(JSON::toString(plugin).toUTF8()).toHexString()) return io_result("Invalid checksum.");

  // push warnings for topo changes
  //for(int m = 0; m < )

  /*
  // good to go, init state to default, 
  // overwrite with old values and report warnings
  for(int m = 0; m < plugin["state"].size(); m++)
  {
    // mapping old to new module
    int new_module_index = -1;
    for(int mn = 0; mn < _topo->modules.size(); mn++)
      if(_topo->modules[mn].id == plugin["modules"][m]["id"])
        new_module_index = mn;
    if (new_module_index == -1)
    {
      auto old_module_name = plugin["modules"][m]["name"].toString().toStdString();
      result.warnings.push_back("Module '" + old_module_name + "' was deleted.");
      continue;
    }

    // check for changed slot count
    if (plugin["state"][m]["slots"].size() != _topo->modules[new_module_index].slot_count)
    {
      auto new_module_name = _topo->modules[new_module_index].name;
      result.warnings.push_back("Module '" + new_module_name + "' changed slot count.");
    }

    for(int mi = 0; mi < plugin["state"][m]["slots"].size() && mi < _topo->modules[new_module_index].slot_count; mi++)
      for(int p = 0; p < plugin["state"][m]["slots"][mi]["params"].size(); p++)
      {
        // mapping old to new parameter
        int new_param_index = -1;
        var old_param = plugin["modules"][m]["params"][p];
        for (int pn = 0; pn < _topo->modules[new_module_index].params.size(); pn++)
          if (_topo->modules[new_module_index].params[pn].id == old_param["id"])
            new_param_index = pn;
        if (new_param_index == -1)
        {
          auto old_param_name = old_param["name"].toString().toStdString();
          result.warnings.push_back("Param '" + old_module_name + " " + old_param_name + "' was deleted.");
          continue;
        }

        // check for changed slot count
        var old_param_slots = plugin["state"][m]["slots"][mi]["params"][p]["slots"];
        auto new_param_name = _topo->modules[new_module_index].params[new_param_index].name;
        if (old_param_slots.size() != _topo->modules[new_module_index].params[new_param_index].slot_count)
          result.warnings.push_back("Param '" + new_module_name + " " + new_param_name +  "' changed slot count.");

        for (int pi = 0; pi < plugin["state"][m]["slots"][mi]["params"][p]["slots"].size(); pi++)
        {
          std::string text = plugin["state"][m]["slots"][mi]["params"][p]["slots"][pi].toString().toStdString();
          (void)text;
        }      
      }
  }
  */

  return result;
}

}