#pragma once

namespace infernal::base {

struct module_topo;
struct plugin_block;

typedef void(*module_process)(
module_topo const& topo, int module_index, plugin_block const& block);

}
