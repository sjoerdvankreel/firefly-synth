#pragma once

#include <plugin_base/topo/plugin.hpp>

// helper functions for audio and cv routing matrices
namespace plugin_base {

template <class M>
struct routing_matrix
{
  std::vector<M> mappings;
  std::vector<plugin_base::list_item> items;
  std::shared_ptr<plugin_base::gui_submenu> submenu;
};

// allows to tidy up cv/audio matrix
class tidy_matrix_menu_handler :
public tab_menu_handler {
  int _on_param;
  int _off_value;
  std::vector<int> _sort_params;
public:
  std::vector<std::string> const extra_items() const override { return { "Tidy", "Sort" }; };
  void extra(plugin_base::plugin_state* state, int module, int slot, int action) override;
  tidy_matrix_menu_handler(int on_param, int off_value, std::vector<int> const& sort_params) : 
  _on_param(on_param), _off_value(off_value), _sort_params(sort_params) {}
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

}