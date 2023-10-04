#pragma once

#include <plugin_base/utility.hpp>
#include <plugin_base/topo/gui.hpp>
#include <plugin_base/topo/module.hpp>

#include <vector>
#include <string>

namespace plugin_base {

enum class plugin_type { synth, fx };

// plugin ui
struct plugin_topo_gui final {
  int min_width;
  int max_width;
  int default_width;
  int aspect_ratio_width;
  int aspect_ratio_height;
  gui_dimension dimension;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(plugin_topo_gui);
};

// plugin definition
struct plugin_topo final {
  std::string id;
  std::string name;
  int version_major;
  int version_minor;

  int polyphony;
  plugin_type type;
  plugin_topo_gui gui;
  std::string extension;
  std::vector<module_topo> modules;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(plugin_topo);
};

}
