#pragma once

#include <plugin_base/topo/module.hpp>
#include <string>

namespace plugin_base {

// stable hash, nonnegative required for vst3 param tags
int 
desc_id_hash(std::string const& text);

std::string 
desc_id(topo_info const& info, int slot);
std::string 
desc_name(topo_info const& info, int slot);

}
