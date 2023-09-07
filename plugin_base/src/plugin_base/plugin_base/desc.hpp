#pragma once
#include <plugin_base/topo.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/param_value.hpp>
#include <map>
#include <vector>
#include <string>

namespace plugin_base {

// mapping plugin level parameter index
struct param_mapping final {
  int group_in_plugin = {};
  int param_in_module = {};
  int module_in_group = {};
  int module_in_plugin = {};

  template <class T> auto& value_at(T& container) const 
  { return container[group_in_plugin][module_in_group][param_in_module]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[group_in_plugin][module_in_group][param_in_module]; }
};

// runtime parameter descriptor
struct param_desc final {
  int id_hash = {};
  std::string id = {};
  std::string name = {};
  param_topo const* topo = {};
  INF_DECLARE_MOVE_ONLY(param_desc);
  param_desc(module_group_topo const& module_group, int module_index, param_topo const& param);
};

// runtime module descriptor
struct module_desc final {
  std::string name = {};
  module_group_topo const* topo = {};
  std::vector<param_desc> params = {};  
  INF_DECLARE_MOVE_ONLY(module_desc);
  module_desc(module_group_topo const& module_group, int module_index);
};

// runtime plugin descriptor
struct plugin_desc final {
  plugin_topo const topo = {};
  std::map<int, int> id_to_index;
  std::vector<module_desc> modules = {};
  std::vector<param_mapping> param_mappings = {};
  INF_DECLARE_MOVE_ONLY(plugin_desc);
  plugin_desc(plugin_topo_factory factory);

  void init_default_state(jarray3d<param_value>& state) const;
  param_desc const& param_at(param_mapping const& mapping) const
  { return modules[mapping.module_in_plugin].params[mapping.param_in_module]; }
};

// runtime plugin topo dimensions
struct plugin_dims final {
  std::vector<int> module_counts;
  std::vector<std::vector<int>> module_param_counts;
  INF_DECLARE_MOVE_ONLY(plugin_dims);
  plugin_dims(plugin_topo const& plugin);
};

// runtime plugin buffer dimensions
struct plugin_frame_dims final {
  std::vector<std::vector<int>> module_cv_frame_counts;
  std::vector<std::vector<std::vector<int>>> module_audio_frame_counts;
  std::vector<std::vector<std::vector<int>>> module_accurate_frame_counts;
  INF_DECLARE_MOVE_ONLY(plugin_frame_dims);
  plugin_frame_dims(plugin_topo const& plugin, int frame_count);
};

}
