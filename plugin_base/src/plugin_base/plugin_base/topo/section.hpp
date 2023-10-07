#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/shared/utility.hpp>

#include <string>

namespace plugin_base {

struct module_topo;

// param section ui
struct section_topo_gui final {
  gui_bindings bindings;
  gui_position position;
  gui_dimension dimension;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(section_topo_gui);
};

// param ui section
struct section_topo final {
  int index;
  topo_tag tag;
  section_topo_gui gui;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(section_topo);
  void validate(module_topo const& module, int index_) const;
};

}
