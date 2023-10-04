#pragma once

#include <plugin_base/utility.hpp>
#include <plugin_base/topo/shared.hpp>

#include <string>

namespace plugin_base {

// param section ui
struct section_topo_gui final {
  gui_bindings bindings;
  gui_position position;
  gui_dimension dimension;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(section_topo_gui);
};

// param ui section
struct section_topo final {
  int index;
  std::string name;
  section_topo_gui gui;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(section_topo);
};

}
