#pragma once

#include <plugin_base/value.hpp>
#include <plugin_base/jarray.hpp>
#include <plugin_base/utility.hpp>
#include <plugin_base/topo/plugin.hpp>

#include <map>
#include <vector>
#include <string>

namespace plugin_base {

struct plugin_desc;

// mapping plugin level parameter index
struct param_mapping final {
  int param_topo = {};
  int param_slot = {};
  int param_local = {};
  int param_global = {};
  int module_topo = {};
  int module_slot = {};
  int module_global = {};

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_mapping);
  template <class T> auto& value_at(T& container) const 
  { return container[module_topo][module_slot][param_topo][param_slot]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_topo][module_slot][param_topo][param_slot]; }
};

// runtime parameter descriptor
struct param_desc final {
  int topo = {};
  int slot = {};
  int local = {};
  int global = {};
  int id_hash = {};
  std::string id = {};
  std::string name = {};
  std::string full_name = {};
  param_topo const* param = {};

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(param_desc);
  param_desc(
    module_topo const& module_, int module_slot,
    param_topo const& param_, int topo, int slot, int local, int global);
};

// runtime module descriptor
struct module_desc final {
  int topo = {};
  int slot = {};
  int global = {};
  int id_hash = {};
  std::string id = {};
  std::string name = {};
  module_topo const* module = {};
  std::vector<param_desc> params = {};

  void validate(plugin_desc const& plugin) const;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(module_desc);

  module_desc(
    module_topo const& module_, int topo, 
    int slot, int global, int param_global_start);
};

// runtime plugin descriptor
struct plugin_desc final {
  int param_count = {};
  int module_count = {};
  int module_voice_start = {};
  int module_output_start = {};
  std::vector<module_desc> modules = {};
  std::vector<param_desc const*> params = {};
  std::unique_ptr<plugin_topo> plugin = {};
  std::vector<param_mapping> mappings = {};
  std::vector<int> param_index_to_tag = {};
  std::map<int, int> param_tag_to_index = {};
  std::map<std::string, int> module_id_to_index = {};
  std::map<std::string, std::map<std::string, int>> param_id_to_index = {};
  std::vector<std::vector<std::vector<std::vector<int>>>> param_topo_to_index = {};

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(plugin_desc);
  plugin_desc(std::unique_ptr<plugin_topo>&& plugin_);

  void init_defaults(jarray<plain_value, 4>& state) const;
  param_desc const& param_at(param_mapping const& mapping) const
  { return modules[mapping.module_global].params[mapping.param_local]; } 
};

}
