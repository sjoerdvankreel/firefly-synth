#pragma once

#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/extra_state.hpp>

#include <vector>
#include <string>
#include <filesystem>

namespace plugin_base {

// per-instance state (plugin parameters and ui specific stuff like selected tabs)

struct load_result
{
  std::string error = {};
  std::vector<std::string> warnings = {};
  bool ok() const { return error.size() == 0; };

  load_result() = default;
  load_result(std::string const& error): error(error) {}
};

class load_handler
{
  juce::var const* _json;
  int const _old_version_major;
  int const _old_version_minor;

public:
  load_handler(juce::var const* json, int old_version_major, int old_version_minor);

  bool const old_version_major() { return _old_version_major; }
  bool const old_version_minor() { return _old_version_minor; }
  bool old_param_value(
    std::string const& module_id, int module_index,
    std::string const& param_id, int param_index,
    std::string& old_value) const;
};

class state_converter
{
public:
  virtual bool 
  handle_invalid_param_value(
    std::string const& module_id, int module_index,
    std::string const& param_id, int param_index,
    load_handler const& handler, plain_value& new_value) = 0;

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