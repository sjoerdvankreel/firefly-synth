#pragma once

#include <plugin_base/topo/module.hpp>
#include <string>

namespace plugin_base {

// stable hash, nonnegative required for vst3 param tags
int 
desc_id_hash(std::string const& text);

std::string 
module_desc_id(module_topo const& module, int slot);
std::string 
module_desc_name(module_topo const& module, int slot);

}
