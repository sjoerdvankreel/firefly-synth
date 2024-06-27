#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <string>

namespace plugin_base {

std::string
user_location(plugin_topo const& topo);

std::string
user_location(std::string const& vendor, std::string const& full_name);

}