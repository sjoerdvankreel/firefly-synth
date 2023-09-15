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
  int param_topo = {};
  int param_slot = {};
  int param_local = {};
  int param_global = {};
  int module_topo = {};
  int module_slot = {};
  int module_global = {};
  INF_DECLARE_MOVE_ONLY(param_mapping);

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
  std::string full_name = {};
  std::string short_name = {};
  param_topo const* param = {};

  INF_DECLARE_MOVE_ONLY(param_desc);
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

  INF_DECLARE_MOVE_ONLY(module_desc);
  module_desc(
    module_topo const& module_, int topo, int slot, int global, int param_global_start);
};

// runtime plugin descriptor
struct plugin_desc final {
  int param_count = {};
  int module_count = {};
  std::vector<int> index_to_id = {};
  std::map<int, int> id_to_index = {};
  std::vector<module_desc> modules = {};
  std::unique_ptr<plugin_topo> plugin = {};
  std::vector<param_mapping> mappings = {};

  INF_DECLARE_MOVE_ONLY(plugin_desc);
  plugin_desc(std::unique_ptr<plugin_topo>&& plugin_);

  void init_defaults(jarray<plain_value, 4>& state) const;
  param_desc const& param_at(param_mapping const& mapping) const
  { return modules[mapping.module_global].params[mapping.param_local]; }

  // utility forwarding to topo
  plain_value raw_to_plain_at(param_mapping const& mapping, double raw) const 
  { return param_at(mapping).param->raw_to_plain(raw); }
  double plain_to_raw_at(param_mapping const& mapping, plain_value plain) const 
  { return param_at(mapping).param->plain_to_raw(plain); }
  normalized_value raw_to_normalized_at(param_mapping const& mapping, double raw) const 
  { return param_at(mapping).param->raw_to_normalized(raw); }
  double normalized_to_raw_at(param_mapping const& mapping, normalized_value normalized) const 
  { return param_at(mapping).param->normalized_to_raw(normalized); }
  normalized_value plain_to_normalized_at(param_mapping const& mapping, plain_value plain) const 
  { return param_at(mapping).param->plain_to_normalized(plain); }
  plain_value normalized_to_plain_at(param_mapping const& mapping, normalized_value normalized) const 
  { return param_at(mapping).param->normalized_to_plain(normalized); }
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
