#pragma once

#include <plugin_base/topo/plugin.hpp>

#include <vector>
#include <functional>

// helper functions for audio and cv routing matrices
namespace plugin_base {

typedef std::function<std::vector<module_topo const*>(plugin_topo const*)>
audio_routes_factory;
typedef std::function<std::vector<module_topo const*>(plugin_topo const*, int module)>
cv_source_routes_factory;

template <class M>
struct routing_matrix
{
  std::vector<M> mappings;
  std::vector<plugin_base::list_item> items;
  std::shared_ptr<plugin_base::gui_submenu> submenu;
};

// any audio module as a source or target
routing_matrix<plugin_base::module_topo_mapping>
make_audio_matrix(std::vector<plugin_base::module_topo const*> const& modules);

// targets any modulatable parameter in modules
routing_matrix<plugin_base::param_topo_mapping>
make_cv_target_matrix(std::vector<plugin_base::module_topo const*> const& modules);

// sources are any cv outputs in modules
routing_matrix<plugin_base::module_output_mapping>
make_cv_source_matrix(std::vector<plugin_base::module_topo const*> const& modules);

// allows to tidy up cv/audio matrix
class tidy_matrix_menu_handler :
public tab_menu_handler {
  int _on_param;
  int _off_value;
  std::vector<int> _sort_params;

public:
  tidy_matrix_menu_handler(int on_param, int off_value, std::vector<int> const& sort_params) : 
  _on_param(on_param), _off_value(off_value), _sort_params(sort_params) {}

  std::vector<std::string> const extra_items() const override { return { "Tidy", "Sort" }; };
  tab_menu_result extra(plugin_base::plugin_state* state, int module, int slot, int action) override;
};

// allows to clear/swap/copy/move with updating routes
class cv_routing_menu_handler :
public tab_menu_handler {
  int const _on_param;
  int const _off_value;
  int const _source_param;
  std::vector<int> const _matrix_modules;
  cv_source_routes_factory const _sources_factory;

  bool is_selected(
    plugin_state* state, int matrix, int param, int route, int module, 
    int slot, std::vector<module_output_mapping> const& mappings);
  bool update_matched_slot(
    plugin_state* state, int matrix, int param, int route, int module,
    int from_slot, int to_slot, std::vector<module_output_mapping> const& mappings);

public:
  bool has_module_menu() const override { return true; }
  std::string module_menu_name() const override { return "With Routing"; };

  cv_routing_menu_handler(std::vector<int> const& matrix_modules, int source_param, 
    int on_param, int off_value, cv_source_routes_factory const& sources_factory):
  _on_param(on_param), _off_value(off_value), _source_param(source_param),
  _matrix_modules(matrix_modules), _sources_factory(sources_factory) {}

  tab_menu_result clear(plugin_state* state, int module, int slot) override;
  tab_menu_result move(plugin_state* state, int module, int source_slot, int target_slot) override;
  tab_menu_result copy(plugin_state* state, int module, int source_slot, int target_slot) override;
  tab_menu_result swap(plugin_state* state, int module, int source_slot, int target_slot) override;
};

// allows to clear/swap/copy/move with updating routes
class audio_routing_menu_handler :
public tab_menu_handler {
  int const _on_param;
  int const _off_value;
  int const _source_param;
  int const _target_param;
  int const _matrix_module;
  audio_routes_factory const _sources_factory;
  audio_routes_factory const _targets_factory;

  bool is_selected(
    plugin_state* state, int param, int route, int module, int slot,
    std::vector<module_topo_mapping> const& mappings);
  bool update_matched_slot(
    plugin_state* state, int param, int route, int module,
    int from_slot, int to_slot, std::vector<module_topo_mapping> const& mappings);

public:
  bool has_module_menu() const override { return true; }
  std::string module_menu_name() const override { return "With Routing"; };

  tab_menu_result clear(plugin_state* state, int module, int slot) override;
  tab_menu_result move(plugin_state* state, int module, int source_slot, int target_slot) override;
  tab_menu_result copy(plugin_state* state, int module, int source_slot, int target_slot) override;
  tab_menu_result swap(plugin_state* state, int module, int source_slot, int target_slot) override;

  audio_routing_menu_handler(
    int matrix_module, int source_param, int target_param, int on_param, int off_value, 
    audio_routes_factory const& sources_factory, audio_routes_factory const& targets_factory):
  _on_param(on_param), _off_value(off_value), _source_param(source_param), _target_param(target_param), 
  _matrix_module(matrix_module), _sources_factory(sources_factory), _targets_factory(targets_factory) {}
};

}