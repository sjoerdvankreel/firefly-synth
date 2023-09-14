#pragma once
#include <plugin_base/topo.hpp>
#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <map>
#include <vector>
#include <string>

namespace plugin_base {

// mapping plugin level parameter index
struct param_mapping final {
  int param_topo_index = {};
  int param_slot_index = {};
  int param_local_index = {};
  int param_global_index = {};
  int module_topo_index = {};
  int module_slot_index = {};
  int module_global_index = {};
  INF_DECLARE_MOVE_ONLY(param_mapping);

  template <class T> auto& value_at(T& container) const 
  { return container[module_topo_index][module_slot_index][param_topo_index][param_slot_index]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_topo_index][module_slot_index][param_topo_index][param_slot_index]; }
};

// runtime parameter descriptor
struct param_desc final {
  int id_hash = {};
  int topo_index = {};
  int slot_index = {};
  int local_index = {};
  int global_index = {};
  std::string id = {};
  std::string full_name = {};
  std::string short_name = {};
  param_topo const* topo = {};

  INF_DECLARE_MOVE_ONLY(param_desc);
  param_desc(
    module_topo const& module, int module_slot_index,
    param_topo const& param, int topo_index, int slot_index, 
    int local_index, int global_index);
};

// runtime module descriptor
struct module_desc final {
  int id_hash = {};
  int topo_index = {};
  int slot_index = {};
  int global_index = {};
  std::string id = {};
  std::string name = {};
  module_topo const* topo = {};
  std::vector<param_desc> params = {};

  INF_DECLARE_MOVE_ONLY(module_desc);
  module_desc(
    module_topo const& module, int topo_index, int slot_index, 
    int global_index, int param_global_index_start);
};

// runtime plugin descriptor
struct plugin_desc final {
  int param_global_count = {};
  int module_global_count = {};
  std::vector<module_desc> modules = {};
  std::unique_ptr<plugin_topo> topo = {};
  std::vector<int> param_index_to_id = {};
  std::map<int, int> param_id_to_index = {};
  std::vector<param_mapping> param_mappings = {};

  INF_DECLARE_MOVE_ONLY(plugin_desc);
  plugin_desc(std::unique_ptr<plugin_topo>&& topo_);

  void init_default_state(jarray<plain_value, 4>& state) const;
  param_desc const& param_at(param_mapping const& mapping) const
  { return modules[mapping.module_global_index].params[mapping.param_local_index]; }
};

// runtime plugin topo dimensions
struct plugin_dims final {
  jarray<int, 3> params;
  jarray<int, 1> modules;

  INF_DECLARE_MOVE_ONLY(plugin_dims);
  plugin_dims(plugin_topo const& topo);
};

// runtime plugin buffer dimensions
struct plugin_frame_dims final {
  jarray<int, 2> cv;
  jarray<int, 3> audio;
  jarray<int, 4> accurate;

  INF_DECLARE_MOVE_ONLY(plugin_frame_dims);
  plugin_frame_dims(plugin_topo const& topo, int frame_count);
};

}
