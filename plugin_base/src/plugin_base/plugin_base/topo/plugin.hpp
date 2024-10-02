#pragma once

#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/dsp/block/host.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstdint>

namespace plugin_base {

class lnf;
class plugin_gui;
class plugin_state;
class arp_engine_base;

struct plugin_desc;
struct plugin_topo_gui;
struct plugin_topo_gui_theme_settings;

enum class plugin_type { synth, fx };

// from resources folder
struct preset_item
{
  std::string name;
  std::string path;
  std::string group;
};

typedef std::function<std::unique_ptr<arp_engine_base>(plugin_topo const*)>
arpeggiator_factory;

// global unison support
typedef int (*sub_voice_counter_t)(bool graph, plugin_state const& state);

typedef std::function<juce::Component&(std::unique_ptr<juce::Component>&&)>
component_store;
typedef std::function<gui_dimension(plugin_topo_gui_theme_settings const& settings)>
plugin_dimension_factory;
typedef std::function<juce::Component&(plugin_gui* gui, lnf* lnf, component_store store)>
custom_gui_factory;

// free-form ui
struct custom_section_gui final {
  int index;
  gui_position position;
  std::string full_name; // needed for theme
  custom_gui_factory gui_factory;

  void validate(plugin_topo const& topo, int index_) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(custom_section_gui);
};

// module ui grouping
struct module_section_gui final {
  int index;
  bool tabbed;
  bool visible;
  bool auto_size_tab_headers = false;

  std::string id;
  gui_position position;
  gui_dimension dimension;
  std::vector<int> tab_order;
  
  void validate(plugin_topo const& plugin, int index_) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_section_gui);
};

// from theme json
struct section_topo_gui_theme_settings final {
  int tab_width = 30;
  int header_width = 80;
};

// from theme json
struct plugin_topo_gui_theme_settings final {

  float lighten = 0.15f;
  float min_scale = 0.5f;
  float max_scale = 8.0f;

  int knob_padding = 5;
  int tabular_knob_padding = 3;
  int linux_font_height = 11;
  int windows_font_height = 13;
  int mac_font_height = 11;

  int combo_radius = 3;
  int section_radius = 4;
  int button_radius = 6;
  int table_cell_radius = 3;
  int scroll_thumb_radius = 2;

  int param_section_vpadding = 0;
  int param_section_radius = 4;
  
  int default_width_fx = 800;
  int aspect_ratio_width_fx = 4;
  int aspect_ratio_height_fx = 3;
  int default_width_instrument = 800;
  int aspect_ratio_width_instrument = 4;
  int aspect_ratio_height_instrument = 3;

  int get_font_height() const
  {
#if WIN32
    return windows_font_height;
#elif (defined __APPLE__)
    return mac_font_height;
#elif (defined __linux__) || (defined  __FreeBSD__)
    return linux_font_height;
#else
#error
#endif
  }

  int get_default_width(bool is_fx) const { return is_fx? default_width_fx: default_width_instrument; }
  int get_aspect_ratio_width(bool is_fx) const { return is_fx? aspect_ratio_width_fx: aspect_ratio_width_instrument; }
  int get_aspect_ratio_height(bool is_fx) const { return is_fx? aspect_ratio_height_fx: aspect_ratio_height_instrument; }
};

// plugin ui
struct plugin_topo_gui final {
  std::string default_theme = {};
  int font_flags = juce::Font::plain;
  plugin_dimension_factory dimension_factory;
  std::vector<custom_section_gui> custom_sections;
  std::vector<module_section_gui> module_sections;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_topo_gui);
};

// stuff that needs cooperation plug<->plugin_base
struct engine_param 
{
  int module_index = -1;
  int param_index = -1;
};

struct engine_params
{
  // realtime visualization
  engine_param visuals = {};

  // smooths parameter automation, midi and bpm changes, use -1 for defaults,
  // must resolve to real parameter indicating nr of milliseconds to smooth
  engine_param bpm_smoothing = {};
  engine_param midi_smoothing = {};
  engine_param automation_smoothing = {};

  // microtuning is done by plugin_base so we need some cooperation
  engine_param tuning_mode = {};

  // voice management is done by plugin_base so we need some cooperation
  engine_param voice_mode = {};
  sub_voice_counter_t sub_voice_counter = {};

  // arpeggiator allows plug to rewrite the note stream
  int arpeggiator_module_index = -1; // nonnegative to activate
  arpeggiator_factory arpeggiator_factory_ = {};
};

// plugin definition
struct plugin_topo final {
  int audio_polyphony;
  int graph_polyphony;
  engine_params engine;
  plugin_version version;

  topo_tag tag;
  plugin_type type;
  std::string vendor;
  std::string full_name;
  plugin_topo_gui gui;
  std::string extension;
  std::vector<module_topo> modules;
  format_basic_config const* config = {};

  void validate() const;
  std::vector<std::string> themes() const;
  std::vector<preset_item> presets() const;
  std::vector<list_item> preset_list() const;
  std::shared_ptr<gui_submenu> preset_submenu() const;

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
