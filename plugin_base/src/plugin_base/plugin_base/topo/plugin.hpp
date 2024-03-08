#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <string>
#include <cstdint>

namespace plugin_base {

class lnf;
class plugin_gui;
class plugin_state;

struct plugin_desc;
struct plugin_topo_gui;
enum class plugin_type { synth, fx };

typedef std::function<juce::Component&(std::unique_ptr<juce::Component>&&)>
component_store;
typedef std::function<juce::Component&(plugin_gui* gui, lnf* lnf, component_store store)>
custom_gui_factory;

// free-form ui
struct custom_section_gui final {
  int index;
  gui_colors colors;
  gui_position position;
  std::string background_image;
  custom_gui_factory gui_factory;

  void validate(plugin_topo const& topo, int index_) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(custom_section_gui);
};

// module ui grouping
struct module_section_gui final {
  int index;
  bool tabbed;
  bool visible;

  std::string id;
  gui_position position;
  gui_dimension dimension;
  std::vector<int> tab_order;
  
  void validate(plugin_topo const& plugin, int index_) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_section_gui);
};

// plugin ui
struct plugin_topo_gui final {
  int default_width;
  float min_scale = 0.5f;
  float max_scale = 2.0f;
  int aspect_ratio_width;
  int aspect_ratio_height;
  gui_colors colors;
  gui_dimension dimension;

  float lighten = 0.15f;
  int font_height = 13;
  int module_tab_width = 30;
  int module_header_width = 80;
  int module_corner_radius = 4;
  int section_corner_radius = 4;
  std::string typeface_file_name;
  int font_flags = juce::Font::plain;

  std::vector<custom_section_gui> custom_sections;
  std::vector<module_section_gui> module_sections;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo_gui);
};

// plugin definition
struct plugin_topo final {
  int audio_polyphony;
  int graph_polyphony;
  plugin_version version;

  topo_tag tag;
  plugin_type type;
  std::string vendor;
  plugin_topo_gui gui;
  std::string extension;
  std::vector<module_topo> modules;

  // smooths midi and bpm changes, use -1 for defaults,
  // must resolve to real parameter indicating nr of milliseconds to smooth
  int bpm_smooth_param = -1;
  int bpm_smooth_module = -1;
  int midi_smooth_param = -1;
  int midi_smooth_module = -1;

  // voice management is done by plugin_base so we need some cooperation
  int voice_mode_param = -1;
  int voice_mode_module = -1;

  void validate() const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo);
};

template <class T, class... U> T&
store_component(component_store store, U&&... args)
{
  auto component = std::make_unique<T>(std::forward<U>(args)...);
  T* result = component.get();
  store(std::move(component));
  return *result;
}

}
