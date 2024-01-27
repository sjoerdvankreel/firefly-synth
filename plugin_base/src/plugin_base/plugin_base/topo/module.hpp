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
struct plugin_desc;

class plugin_state;
class graph_engine;
class module_engine;

enum class module_output { none, cv, audio };
enum class module_stage { input, voice, output };

class module_tab_menu_result {
  bool _show_warning = false;
  std::string _item = "";
  std::string _title = "";
  std::string _content = "";

public:
  bool show_warning() const { return _show_warning; }
  std::string const& item() const { return _item; }
  std::string const& title() const { return _title; }
  std::string const& content() const { return _content; }

  module_tab_menu_result(module_tab_menu_result const&) = default;
  module_tab_menu_result(std::string const& item, bool show_warning, std::string const& title, std::string const& content):
  _item(item), _show_warning(show_warning), _title(title), _content(content) {}
};

// allows to extend right-click menu on tab headers
class module_tab_menu_handler {
protected:
  plugin_state* const _state;
  module_tab_menu_handler(plugin_state* state): _state(state) {}

public:
  struct module_menu { int menu_id; std::string name; std::set<int> actions; };
  enum module_action { clear = 1, clear_all, delete_, insert_before, insert_after, copy_to, move_to, swap_with };

  virtual ~module_tab_menu_handler() {}
  virtual std::vector<module_menu> module_menus() const { return {}; };
  virtual std::vector<custom_menu> const custom_menus() const { return {}; };

  // pop up a message box if these return a warning text
  virtual module_tab_menu_result execute_custom(int menu_id, int action, int module, int slot)
  { return module_tab_menu_result("", false, "", ""); }
  virtual module_tab_menu_result execute_module(int menu_id, int action, int module, int source_slot, int target_slot) 
  { return module_tab_menu_result("", false, "", ""); }
};

typedef std::function<void(plugin_state& state)>
state_initializer;
typedef std::function<std::unique_ptr<graph_engine>(plugin_desc const* desc)>
module_graph_engine_factory;
typedef std::function<std::unique_ptr<module_tab_menu_handler>(plugin_state*)>
module_tab_menu_handler_factory;

typedef std::function<void(
  plugin_state const& state, int slot, jarray<int, 3>& active)>
midi_active_selector;
typedef std::function<std::unique_ptr<module_engine>(
  plugin_topo const& topo, int sample_rate, int max_frame_count)> 
module_engine_factory;
typedef std::function<graph_data(
  plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)>
module_graph_renderer;

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
  int autofit_row = 0;
  int autofit_column = 0;
  bool enable_tab_menu = true;
  module_tab_menu_handler_factory menu_handler_factory;
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
  midi_active_selector midi_active_selector_;

  module_graph_renderer graph_renderer;
  module_engine_factory engine_factory;
  module_graph_engine_factory graph_engine_factory;

  state_initializer minimal_initializer;
  state_initializer default_initializer;
  bool force_rerender_on_param_hover = false;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_topo);
  void validate(plugin_topo const& plugin, int index) const;
};

}
