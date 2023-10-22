#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/utility.hpp>

#include <vector>
#include <string>

namespace plugin_base {

struct plugin_topo_gui;
enum class plugin_type { synth, fx };

// module ui grouping
struct module_section_gui final {
  gui_position position;
  gui_dimension dimension;

  void validate(plugin_topo const& plugin) const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_section_gui);
};

// plugin ui
struct plugin_topo_gui final {
  int min_width;
  int max_width;
  int default_width;

  int aspect_ratio_width;
  int aspect_ratio_height;
  gui_dimension dimension;
  std::vector<module_section_gui> sections;

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
