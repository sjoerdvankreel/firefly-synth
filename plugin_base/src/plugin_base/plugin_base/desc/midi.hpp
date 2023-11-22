#pragma once

#include <plugin_base/desc/shared.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/topo/midi_source.hpp>

#include <string>

namespace plugin_base {

struct module_desc;

// runtime midi source descriptor
struct midi_desc final {
  int local = {};
  desc_info info = {};
  std::string full_name = {};
  midi_source const* source = {};

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_desc);
  void validate(module_desc const& module, int index) const;

  midi_desc(
    module_topo const& module_, int module_slot,
    midi_source const& source_, int topo, int local_, int global);
};

}
