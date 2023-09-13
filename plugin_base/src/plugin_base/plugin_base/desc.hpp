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
  int param_index_in_topo = {};
  int param_topo_index_in_module = {};
  int module_index_in_topo = {};
  int module_topo_index_in_plugin = {};

  template <class T> auto& value_at(T& container) const 
  { return container[module_topo_index_in_plugin][module_index_in_topo]
                    [param_topo_index_in_module][param_index_in_topo]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_topo_index_in_plugin][module_index_in_topo]
                    [param_topo_index_in_module][param_index_in_topo]; }
};

// runtime parameter descriptor
struct param_desc final {
  int id_hash = {};
  std::string id = {};
  std::string name = {};
  param_topo const* topo = {};
  int global_param_index = {};
  int param_index_in_topo = {};
  int topo_index_in_module = {};
  INF_DECLARE_MOVE_ONLY(param_desc);
  param_desc(
    module_topo const& module, param_topo const& param, 
    int global_param_index, int module_index_in_topo, 
    int topo_index_in_module, int param_index_in_topo);
};

// runtime module descriptor
struct module_desc final {
  int id_hash = {};
  std::string id = {};
  std::string name = {};
  module_topo const* topo = {};
  int global_module_index = {};
  int module_index_in_topo = {};
  int topo_index_in_plugin = {};
  std::vector<param_desc> params = {};
  INF_DECLARE_MOVE_ONLY(module_desc);
  module_desc(
    module_topo const& module, 
    int global_module_index, int global_param_index_start,
    int topo_index_in_plugin, int module_index_in_topo);
};

// runtime plugin descriptor
struct plugin_desc final {
  plugin_topo const topo = {};
  int global_param_count = {};
  int global_module_count = {};
  std::vector<module_desc> modules = {};
  std::vector<param_mapping> global_param_mappings = {};
  std::vector<int> global_param_index_to_param_id = {};
  std::map<int, int> param_id_to_global_param_index = {};
  INF_DECLARE_MOVE_ONLY(plugin_desc);
  plugin_desc(plugin_topo_factory factory);

  void init_default_state(jarray4d<plain_value>& state) const;
  param_desc const& param_at(param_mapping const& mapping) const
  { return modules[mapping.module_topo_index_in_plugin].params[mapping.param_topo_index_in_module]; }
};

// runtime plugin topo dimensions
struct plugin_dims final {
  std::vector<int> module_counts;
  std::vector<std::vector<std::vector<int>>> module_param_counts;
  INF_DECLARE_MOVE_ONLY(plugin_dims);
  plugin_dims(plugin_topo const& plugin);
};

// runtime plugin buffer dimensions
struct plugin_frame_dims final {
  std::vector<std::vector<int>> module_cv_frame_counts;
  std::vector<std::vector<std::vector<int>>> module_audio_frame_counts;
  std::vector<std::vector<std::vector<std::vector<int>>>> module_accurate_frame_counts;
  INF_DECLARE_MOVE_ONLY(plugin_frame_dims);
  plugin_frame_dims(plugin_topo const& plugin, int frame_count);
};

}
