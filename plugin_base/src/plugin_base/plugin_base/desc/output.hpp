#pragma once

#include <plugin_base/desc/shared.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/topo/module.hpp>

#include <string>

namespace plugin_base {

struct module_desc;

// runtime output source descriptor
struct output_desc final {
  int local = {};
  topo_desc_info info = {};
  std::string full_name = {};
  module_dsp_output const* source = {};

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(output_desc);
  void validate(module_desc const& module, int index) const;

  output_desc(
    module_topo const& module_, int module_slot,
    module_dsp_output const& source_, int topo, int slot, int local_, int global);
};

}
