#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/utility.hpp>

#include <vector>
#include <cstdint>

namespace plugin_base {

enum midi_message { midi_msg_cc = 176, midi_msg_cp = 208, midi_msg_pb = 224 };

// midi topo mapping
struct midi_topo_mapping final {
  int module_index;
  int module_slot;
  int midi_index;

  template <class T> auto& value_at(T& container) const
  { return container[module_index][module_slot][midi_index]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_index][module_slot][midi_index]; }
};

// mapping midi inputs to continuous series
// message must be 0-127 for cc, midi_msg_cp or midi_msg_pb
struct midi_source final {
  int message;
  topo_tag tag;
  float default_;

  void validate() const;
  static std::vector<midi_source> all_sources();
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_source);
};

}
