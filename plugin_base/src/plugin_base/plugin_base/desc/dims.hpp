#pragma once

#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/topo/plugin.hpp>

namespace plugin_base {

// runtime plugin dsp dimensions
struct plugin_dims final {
  jarray<int, 1> module_slot;
  jarray<int, 2> voice_module_slot;
  jarray<int, 3> module_slot_param_slot;

  plugin_dims(plugin_topo const& plugin);
  void validate(plugin_topo const& plugin) const;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(plugin_dims);
};

}
