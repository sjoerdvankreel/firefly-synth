#pragma once

#include <plugin_base/desc.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

#include <vector>
#include <string>
#include <filesystem>

namespace plugin_base {

struct io_load
{
  std::string error = {};
  jarray<plain_value, 4> state = {};
  std::vector<std::string> warnings = {};
  bool ok() { return error.size() == 0; };

  io_load() = default;
  io_load(std::string const& error): error(error) {}
};

class plugin_io final
{
  plugin_desc const* const _desc;
public:
  INF_DECLARE_MOVE_ONLY(plugin_io);
  plugin_io(plugin_desc const* desc): _desc(desc) {}

  io_load load(std::vector<char> const& data) const;
  io_load load_file(std::filesystem::path const& path) const;
  std::vector<char> save(jarray<plain_value, 4> const& state) const;
  bool save_file(std::filesystem::path const& path, jarray<plain_value, 4> const& state) const;
};

}