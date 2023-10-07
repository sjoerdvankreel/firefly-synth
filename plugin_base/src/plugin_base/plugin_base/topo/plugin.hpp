#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/utility.hpp>

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

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo_gui);
};

// plugin definition
struct plugin_topo final {
  int polyphony;
  int version_major;
  int version_minor;

  topo_tag tag;
  plugin_type type;
  plugin_topo_gui gui;
  std::string extension;
  std::vector<module_topo> modules;

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo);
};

}
