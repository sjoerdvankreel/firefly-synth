#pragma once

#include <plugin_base/utility.hpp>
#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/shared.hpp>
#include <plugin_base/topo/module.hpp>

#include <vector>
#include <string>

namespace plugin_base {

struct plugin_desc;

// runtime module descriptor
struct module_desc final {
  desc_info info = {};
  module_topo const* module = {};
  std::vector<param_desc> params = {};

  void validate(plugin_desc const& plugin) const;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(module_desc);

  module_desc(
    module_topo const& module_, int topo,
    int slot, int global, int param_global_start);
};

}
