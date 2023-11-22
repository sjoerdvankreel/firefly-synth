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