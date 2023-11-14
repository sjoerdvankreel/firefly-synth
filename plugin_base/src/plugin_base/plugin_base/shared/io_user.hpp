#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <string>

namespace plugin_base {

enum class user_io { base, plugin };

void user_io_save_num(plugin_topo const& topo, user_io where, std::string const& key, double val);
void user_io_save_text(plugin_topo const& topo, user_io where, std::string const& key, std::string const& val);
std::string user_io_load_text(plugin_topo const& topo, user_io where, std::string const& key, std::string const& default_);
double user_io_load_num(plugin_topo const& topo, user_io where, std::string const& key, double min, double max, double default_);

}