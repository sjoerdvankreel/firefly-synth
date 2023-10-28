#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/utility.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>

namespace plugin_base {

struct plugin_topo_gui;
enum class plugin_type { synth, fx };

// module ui grouping
struct module_section_gui final {
  int index;
  gui_position position;
  gui_dimension dimension;

  void validate(plugin_topo const& plugin, int index_) const;
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

  float lighten = 0.15f;
  int font_height = 13;
  int module_tab_width = 30;
  int module_header_width = 60;
  int module_corner_radius = 4;
  int font_flags = juce::Font::plain;
  std::string font_typeface = "Handel Gothic";

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo_gui);
  juce::Font font() const { return juce::Font(font_typeface, font_height, font_flags); }
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
