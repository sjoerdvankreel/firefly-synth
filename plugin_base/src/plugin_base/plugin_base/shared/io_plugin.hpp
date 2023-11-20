#pragma once

#include <plugin_base/shared/state.hpp>

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

std::vector<char> plugin_io_save(plugin_state const& state);
load_result plugin_io_load(std::vector<char> const& data, plugin_state& state);
bool plugin_io_save_file(std::filesystem::path const& path, plugin_state const& state);
load_result plugin_io_load_file(std::filesystem::path const& path, plugin_state& state);

}