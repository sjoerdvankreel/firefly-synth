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

// any audio module as a source or target
routing_matrix<module_topo_mapping>
make_audio_matrix(std::vector<module_topo const*> const& modules);

// sources are any cv outputs in modules
routing_matrix<module_output_mapping>
make_cv_source_matrix(std::vector<cv_source_entry> const& entries);

// targets any modulatable parameter in modules
routing_matrix<param_topo_mapping>
make_cv_target_matrix(std::vector<module_topo const*> const& modules);

// allows to tidy up cv/audio matrix
class tidy_matrix_menu_handler :
public tab_menu_handler {
  int _on_param;
  int _off_value;
  std::vector<int> _sort_params;

public:
  std::vector<extra_menu> const extra_menus() const override;
  menu_result execute_extra(int menu_id, int action, int module, int slot) override;
  tidy_matrix_menu_handler(plugin_state* state, int on_param, int off_value, std::vector<int> const& sort_params) : 
  tab_menu_handler(state), _on_param(on_param), _off_value(off_value), _sort_params(sort_params) {}
};

// allows to clear/swap/copy/move with updating routes
class cv_routing_menu_handler :
public tab_menu_handler {
  int const _on_param;
  int const _off_value;
  int const _source_param;
  std::map<int, std::vector<module_output_mapping>> const _matrix_sources;

  bool is_selected(
    int matrix, int param, int route, int module, 
    int slot, std::vector<module_output_mapping> const& mappings);
  bool update_matched_slot(
    int matrix, int param, int route, int module, int from_slot, 
    int to_slot, std::vector<module_output_mapping> const& mappings);

public:
#if 0
  std::vector<std::string> module_menu_names() const override { return { "With CV Routing" }; };

  tab_menu_result clear_all(int menu, int module) override;
  tab_menu_result clear(int menu, int module, int slot) override;
  tab_menu_result insert_after(int menu, int module, int slot) override;
  tab_menu_result insert_before(int menu, int module, int slot) override;
  tab_menu_result move(int menu, int module, int source_slot, int target_slot) override;
  tab_menu_result copy(int menu, int module, int source_slot, int target_slot) override;
  tab_menu_result swap(int menu, int module, int source_slot, int target_slot) override;
#endif

  cv_routing_menu_handler(plugin_state* state, int source_param, int on_param, 
    int off_value, std::map<int, std::vector<module_output_mapping>> const& matrix_sources):
  tab_menu_handler(state), _on_param(on_param), _off_value(off_value), 
  _source_param(source_param), _matrix_sources(matrix_sources) {}
};

// allows to clear/swap/copy/move with updating routes
class audio_routing_menu_handler :
public tab_menu_handler {
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

public:
#if 0
  std::vector<std::string> module_menu_names() const override { return { "With CV Routing", "With CV & Audio Routing" }; };

  tab_menu_result clear_all(int menu, int module) override;
  tab_menu_result clear(int menu, int module, int slot) override;
  tab_menu_result move(int menu, int module, int source_slot, int target_slot) override;
  tab_menu_result copy(int menu, int module, int source_slot, int target_slot) override;
  tab_menu_result swap(int menu, int module, int source_slot, int target_slot) override;
#endif

  audio_routing_menu_handler(plugin_state* state, 
    audio_routing_cv_params const& cv_params, std::vector<audio_routing_audio_params> const& audio_params):
  tab_menu_handler(state), _cv_params(cv_params), _audio_params(audio_params) {}
};

}