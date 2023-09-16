#pragma once

#include <plugin_base/topo.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>

#include <vector>
#include <string>
#include <filesystem>

namespace plugin_base {

struct io_result
{
  std::string error;
  std::vector<std::string> warnings;
  bool ok() { return error.size() == 0; };

  io_result() = default;
  io_result(std::string const& error): error(error) {}
};

class plugin_io final
{
  plugin_topo const* const _topo;
public:
  INF_DECLARE_MOVE_ONLY(plugin_io);
  plugin_io(plugin_topo const* topo): _topo(topo) {}

  std::vector<char> save(jarray<plain_value, 4> const& state) const;
  io_result load(std::vector<char> const& data, jarray<plain_value, 4>& state) const;
  io_result load_file(std::filesystem::path const& path, jarray<plain_value, 4>& state) const;
  bool save_file(std::filesystem::path const& path, jarray<plain_value, 4> const& state) const;
};

}