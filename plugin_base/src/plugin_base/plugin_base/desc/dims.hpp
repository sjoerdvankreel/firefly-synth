#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

namespace plugin_base {

// runtime plugin dsp dimensions
struct plugin_dims final {
  jarray<int, 1> module_slot;
  jarray<int, 2> module_slot_midi;
  jarray<int, 2> voice_module_slot;
  jarray<int, 3> module_slot_param_slot;

  plugin_dims(plugin_topo const& plugin);
  void validate(plugin_topo const& plugin) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_dims);
};

}
