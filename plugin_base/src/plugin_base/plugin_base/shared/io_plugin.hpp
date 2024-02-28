#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/extra_state.hpp>

#include <vector>
#include <string>
#include <filesystem>

namespace plugin_base {

// per-instance state (plugin parameters and ui specific stuff like selected tabs)

// ok, error, or ok with warnings
struct load_result
{
  std::string error = {};
  std::vector<std::string> warnings = {};
  bool ok() const { return error.size() == 0; };

  load_result() = default;
  load_result(std::string const& error): error(error) {}
};

// allows file format conversions to read old state
class load_handler
{
  juce::var const* _json;
  plugin_version const _old_version;

public:
  plugin_version const& old_version() const { return _old_version; }
  load_handler(juce::var const* json, plugin_version const& old_version);

  bool old_param_value(
    std::string const& old_module_id, int old_module_slot,
    std::string const& old_param_id, int old_param_slot,
    std::string& old_value) const;
};

// allows individual modules to perform file format conversion
class state_converter
{
public:
  virtual bool 
  handle_invalid_param_value(
    std::string const& new_module_id, int new_module_slot,
    std::string const& new_param_id, int new_param_slot,
    std::string const& old_value, load_handler const& handler, 
    plain_value& new_value) = 0;

  virtual void
  post_process(load_handler const& handler, plugin_state& new_state) = 0;
};

std::vector<char> plugin_io_save_state(plugin_state const& state);
load_result plugin_io_load_state(std::vector<char> const& data, plugin_state& state);
std::vector<char> plugin_io_save_extra(plugin_topo const& topo, extra_state const& state);
load_result plugin_io_load_extra(plugin_topo const& topo, std::vector<char> const& data, extra_state& state);

std::vector<char> plugin_io_save_all(plugin_state const& plugin, extra_state const& extra);
load_result plugin_io_load_all(std::vector<char> const& data, plugin_state& plugin, extra_state& extra);
load_result plugin_io_load_file_all(std::filesystem::path const& path, plugin_state& plugin, extra_state& extra);
bool plugin_io_save_file_all(std::filesystem::path const& path, plugin_state const& plugin, extra_state const& extra);

}