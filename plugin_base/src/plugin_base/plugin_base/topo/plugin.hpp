#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/utility.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <string>
#include <cstdint>

namespace plugin_base {

struct plugin_topo_gui;
enum class plugin_type { synth, fx };

// module ui grouping
struct module_section_gui final {
  int index;
  bool tabbed;
  bool visible;
  int tab_width;
  gui_position position;
  gui_dimension dimension;
  std::string tab_header;
  std::vector<int> tab_order;
  
  void validate(plugin_topo const& plugin, int index_) const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_section_gui);
};

// plugin ui
struct plugin_topo_gui final {
  int min_width;
  int max_width;

  gui_colors colors;
  int aspect_ratio_width;
  int aspect_ratio_height;
  gui_dimension dimension;
  std::vector<module_section_gui> sections;

  float lighten = 0.15f;
  int font_height = 13;
  int module_tab_width = 30;
  int module_header_width = 60;
  int module_corner_radius = 4;
  int section_corner_radius = 4;
  std::string typeface_file_name;
  int font_flags = juce::Font::plain;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo_gui);
};

// plugin definition
struct plugin_topo final {
  int polyphony;
  int version_major;
  int version_minor;
  float midi_smoothing_hz = 20;

  topo_tag tag;
  plugin_type type;
  plugin_topo_gui gui;
  std::string extension;
  std::vector<module_topo> modules;

  void validate() const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo);
};

}
