#pragma once

#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/section.hpp>
#include <plugin_base/topo/midi_source.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/shared/graph_data.hpp>

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>

namespace plugin_base {

struct plugin_topo;
class plugin_state;
class module_engine;

enum class module_output { none, cv, audio };
enum class module_stage { input, voice, output };

struct tab_menu_result
{
  bool show_warning;
  std::string title;
  std::string content;
};

// allows to extend right-click menu on tab headers
class tab_menu_handler {
protected:
  plugin_state* const _state;
  tab_menu_handler(plugin_state* state): _state(state) {}

public:
  virtual ~tab_menu_handler() {}
  virtual std::vector<std::string> module_menu_names() const { return {}; };
  virtual std::vector<std::string> const extra_items() const { return {}; };

  // pop up a message box if these return a non-empty text
  virtual tab_menu_result extra(int module, int slot, int action) { return {}; };
  virtual tab_menu_result clear(int menu, int module, int slot) { return {}; };
  virtual tab_menu_result move(int menu, int module, int source_slot, int target_slot) { return {}; };
  virtual tab_menu_result copy(int menu, int module, int source_slot, int target_slot) { return {}; };
  virtual tab_menu_result swap(int menu, int module, int source_slot, int target_slot) { return {}; };
};

typedef std::function<void(plugin_state& state)>
state_initializer;

typedef std::function<void(
  plugin_state const& state, int slot, jarray<int, 3>& active)>
midi_active_selector;

typedef std::function<graph_data(
  plugin_state const& state, param_topo_mapping const& mapping)>
module_graph_renderer;

typedef std::function<std::unique_ptr<module_engine>(
  plugin_topo const& topo, int sample_rate, int max_frame_count)> 
module_engine_factory;

typedef std::function<std::unique_ptr<tab_menu_handler>(plugin_state*)>
tab_menu_handler_factory;

// module topo mapping
struct module_topo_mapping final {
  int index;
  int slot;
};

// module topo mapping
struct module_output_mapping final {
  int module_index;
  int module_slot;
  int output_index;
  int output_slot;
};

// module ui
struct module_topo_gui final {
  int section;
  bool visible;
  gui_colors colors;
  gui_position position;
  gui_dimension dimension;
  std::string tabbed_name;
  bool enable_tab_menu = true;
  tab_menu_handler_factory menu_handler_factory;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_topo_gui);
};

// module output
struct module_dsp_output final {
  topo_info info;
  bool is_modulation_source;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_dsp_output);
};

// module dsp
struct module_dsp final {
  int scratch_count;
  module_stage stage;
  module_output output;
  std::vector<module_dsp_output> outputs;

  void validate() const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_dsp);
};

// module in plugin
struct module_topo final {
  module_dsp dsp;
  topo_info info;
  module_topo_gui gui;
  std::vector<param_topo> params;
  std::vector<param_section> sections;
  std::vector<midi_source> midi_sources;
  midi_active_selector midi_active_selector;

  bool rerender_on_param_hover = false;
  module_graph_renderer graph_renderer;
  module_engine_factory engine_factory;
  state_initializer minimal_initializer;
  state_initializer default_initializer;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_topo);
  void validate(plugin_topo const& plugin, int index) const;
};

}
