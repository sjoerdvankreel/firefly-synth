#pragma once

#include <plugin_base/topo.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <vector>
#include <cstdint>

namespace plugin_base {

std::vector<std::uint8_t> io_store(plugin_topo const& topo, jarray4d<plain_value> const& state);
bool io_load_file(plugin_topo const& topo, std::string const& path, jarray4d<plain_value>& state);
void io_store_file(plugin_topo const& topo, jarray4d<plain_value> const& state, std::string const& path);
bool io_load(plugin_topo const& topo, std::vector<std::uint8_t> const& blob, jarray4d<plain_value>& state);

}