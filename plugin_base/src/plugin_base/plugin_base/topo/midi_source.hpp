#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/utility.hpp>
#include <cstdint>

namespace plugin_base {

// vst3 kind of limits what we can do here.
// basically amounts to all CC's plus some predefined messages
enum midi_message { midi_msg_cc = 176, midi_msg_cp = 208, midi_msg_pb = 224 };

// midi automation identifier
struct midi_id
{
  std::int16_t message;
  std::int16_t cc_number;

  bool operator<(midi_id const& rhs) const 
  { return message < rhs.message && cc_number < rhs.cc_number; }
};

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
struct midi_source final {
  midi_id id;
  topo_tag tag;

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_source);
};

}
