#pragma once

#include <plugin_base/topo.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <vector>
#include <filesystem>

namespace plugin_base {

std::vector<char> io_store(plugin_topo const& topo, jarray4d<plain_value> const& state);
bool io_load(plugin_topo const& topo, std::vector<char> const& blob, jarray4d<plain_value>& state);
bool io_load_file(plugin_topo const& topo, std::filesystem::path const& path, jarray4d<plain_value>& state);
bool io_store_file(plugin_topo const& topo, jarray4d<plain_value> const& state, std::filesystem::path const& path);

}