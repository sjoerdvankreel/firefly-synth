#pragma once

#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/utility.hpp>

#include <vector>
#include <string>

namespace plugin_base {

struct plugin_desc;

// runtime module descriptor
struct module_desc final {
  desc_info info = {};
  module_topo const* module = {};
  std::vector<param_desc> params = {};

  module_desc(
    module_topo const& module_, int topo,
    int slot, int global, int param_global_start);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_desc);
  void validate(plugin_desc const& plugin, int index) const;
};

}
