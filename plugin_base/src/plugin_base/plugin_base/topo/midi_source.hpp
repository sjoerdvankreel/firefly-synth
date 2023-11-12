#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/utility.hpp>

#include <vector>
#include <cstdint>

namespace plugin_base {

// mapping to vst3
enum midi_source_id { midi_source_cc = 0, midi_source_cp = 128, midi_source_pb = 129, midi_source_count };

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
// id must be 0-127 for cc, midi_msg_cp or midi_msg_pb
struct midi_source final {
  int id;
  topo_tag tag;
  float default_;

  void validate() const;
  static std::vector<midi_source> all_sources();
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_source);
};

}
