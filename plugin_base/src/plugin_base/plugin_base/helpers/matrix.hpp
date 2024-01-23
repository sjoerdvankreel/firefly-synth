#pragma once

#include <plugin_base/topo/plugin.hpp>

#include <vector>
#include <string>
#include <functional>

// helper functions for audio and cv routing matrices
namespace plugin_base {

template <class M>
struct routing_matrix
{
  std::vector<M> mappings;
  std::vector<list_item> items;
  std::shared_ptr<gui_submenu> submenu;
};

// for make_cv_source_matrix
struct cv_source_entry
{
  std::string subheader;
  module_topo const* module;
};

// for audio_routing_menu_handler
struct audio_routing_cv_params
{
  int on_param;
  int off_value;
  int target_param;
  int matrix_module;
  std::vector<param_topo_mapping> targets;
};

// for audio_routing_menu_handler
struct audio_routing_audio_params
{
  int on_param;
  int off_value;
  int source_param;
  int target_param;
  int matrix_module;
  std::vector<module_topo_mapping> sources;
  std::vector<module_topo_mapping> targets;
};

// sources are any cv outputs in modules
routing_matrix<module_output_mapping>
make_cv_source_matrix(std::vector<cv_source_entry> const& entries);
// targets any modulatable parameter in modules
routing_matrix<param_topo_mapping>
make_cv_target_matrix(std::vector<module_topo const*> const& modules);
// any audio module as a source or target
routing_matrix<module_topo_mapping>
make_audio_matrix(std::vector<module_topo const*> const& modules, int start_slot);

// allows to manage matrix routes
class matrix_param_menu_handler:
public param_menu_handler {

  // sections assume equal param and route count
  int const _route_count;
  int const _this_section;
  int const _section_count;
  int const _default_on_value;
  enum { clear, delete_, duplicate, insert_before, insert_after, action_count  };

public:
  matrix_param_menu_handler(plugin_state* state, int section_count, int this_section, int route_count, int default_on_value):
  param_menu_handler(state), _this_section(this_section), _section_count(section_count), _route_count(route_count), _default_on_value(default_on_value) {}
  std::vector<custom_menu> const menus() const override;
  void execute(int menu_id, int action, int module_index, int module_slot, int param_index, int param_slot) override;
};

inline std::unique_ptr<matrix_param_menu_handler>
make_matrix_param_menu_handler(plugin_state* state, int section_count, int this_section, int route_count, int default_on_value)
{ return std::make_unique<matrix_param_menu_handler>(state, section_count, this_section, route_count, default_on_value); }

// allows to tidy up cv/audio matrix
class tidy_matrix_menu_handler :
public module_tab_menu_handler {
  int _on_param;
  int _off_value;

  // for multi-section matrix (am/fm), assumes equal route count and equal param count per section
  // sort params are per-section
  int _section_count;
  std::vector<std::vector<int>> _sort_params;

public:
  std::vector<custom_menu> const custom_menus() const override;
  menu_result execute_custom(int menu_id, int action, int module, int slot) override;
  tidy_matrix_menu_handler(plugin_state* state, int section_count, int on_param, int off_value, std::vector<std::vector<int>> const& sort_params) :
    module_tab_menu_handler(state), _on_param(on_param), _off_value(off_value), _section_count(section_count), _sort_params(sort_params) {}
};

// allows to clear/swap/copy/move with updating routes
class cv_routing_menu_handler :
public module_tab_menu_handler {
  int const _on_param;
  int const _off_value;
  int const _source_param;
  std::map<int, std::vector<module_output_mapping>> const _matrix_sources;

  void clear_all(int module);
  void clear(int module, int slot);
  void delete_(int module, int slot);
  void insert(int module, int slot, bool after);
  void move_to(int module, int source_slot, int target_slot);
  void swap_with(int module, int source_slot, int target_slot);
  void insert_after(int module, int slot) { insert(module, slot, true); }
  void insert_before(int module, int slot) { insert(module, slot, false); }

  bool is_selected(
    int matrix, int param, int route, int module, 
    int slot, std::vector<module_output_mapping> const& mappings);
  bool update_matched_slot(
    int matrix, int param, int route, int module, int from_slot, 
    int to_slot, std::vector<module_output_mapping> const& mappings);

public:
  std::vector<module_menu> module_menus() const override;
  menu_result execute_module(int menu_id, int action, int module, int source_slot, int target_slot) override;

  cv_routing_menu_handler(plugin_state* state, int source_param, int on_param, 
    int off_value, std::map<int, std::vector<module_output_mapping>> const& matrix_sources):
    module_tab_menu_handler(state), _on_param(on_param), _off_value(off_value),
  _source_param(source_param), _matrix_sources(matrix_sources) {}
};

// allows to clear/swap/copy/move with updating routes
class audio_routing_menu_handler :
public module_tab_menu_handler {
  audio_routing_cv_params const _cv_params;
  std::vector<audio_routing_audio_params> const _audio_params;

  bool is_cv_selected(
    int route, int module, int slot,
    std::vector<param_topo_mapping> const& mappings);
  bool update_matched_cv_slot(
    int route, int module, int from_slot, int to_slot);

  bool is_audio_selected(
    int matrix, int param, int route, int module, int slot,
    std::vector<module_topo_mapping> const& mappings);
  bool update_matched_audio_slot(
    int matrix, int param, int route, int module, int from_slot,
    int to_slot, std::vector<module_topo_mapping> const& mappings);

  void clear_cv_route(int route);
  void move_audio_to(int module, int source_slot, int target_slot);

  void with_all_clear_all(int module);
  void with_all_clear(int module, int slot);
  void with_all_delete(int module, int slot);
  void with_all_insert(int module, int slot, bool after);
  void with_all_insert_after(int module, int slot) { with_all_insert(module, slot, true); }
  void with_all_insert_before(int module, int slot) { with_all_insert(module, slot, false); }

  void with_cv_move_to(int module, int source_slot, int target_slot);
  void with_cv_swap_with(int module, int source_slot, int target_slot);
  menu_result with_cv_copy_to(int module, int source_slot, int target_slot);

public:
  std::vector<module_menu> module_menus() const override;
  menu_result execute_module(int menu_id, int action, int module, int source_slot, int target_slot) override;

  audio_routing_menu_handler(plugin_state* state, 
    audio_routing_cv_params const& cv_params, std::vector<audio_routing_audio_params> const& audio_params):
  module_tab_menu_handler(state), _cv_params(cv_params), _audio_params(audio_params) {}
};

}