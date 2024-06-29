#pragma once

#include <cstdlib>

// Generates parameter reference. Ideally this would be in an external tool
// but since we static link MSVCRT that would require a C API wrapping the plugin_topo.

namespace plugin_base {

struct plugin_topo;

// Generate a string, then copy it back to caller.
typedef void(*plugin_topo_on_reference_generated)(char const* text, void* ctx);
void plugin_topo_generate_reference(
  plugin_base::plugin_topo const* topo, 
  plugin_topo_on_reference_generated on_generated, void* ctx);

}