#pragma once

#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

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

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_mapping);
  template <class T> auto& value_at(T& container) const 
  { return container[module_topo][module_slot][param_topo][param_slot]; }
  template <class T> auto const& value_at(T const& container) const 
  { return container[module_topo][module_slot][param_topo][param_slot]; }
};

// mapping to/from global parameter info
struct plugin_param_mappings final {
  std::vector<int> index_to_tag = {};
  std::map<int, int> tag_to_index = {};
  std::vector<param_mapping> params = {};
  std::map<std::string, std::map<std::string, int>> id_to_index = {};
  std::vector<std::vector<std::vector<std::vector<int>>>> topo_to_index = {};

  void validate(plugin_desc const& plugin) const;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_param_mappings);
};

// runtime plugin descriptor
struct plugin_desc final {
  int param_count = {};
  int module_count = {};
  int module_voice_start = {};
  int module_output_start = {};

  plugin_topo const* plugin = {};
  plugin_param_mappings mappings = {};
  std::vector<module_desc> modules = {};
  std::vector<param_desc const*> params = {};
  std::map<std::string, int> module_id_to_index = {};

  void validate() const;
  plugin_desc(plugin_topo const* plugin);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_desc);

  // TODO drop this
  param_desc const& param_at(param_mapping const& mapping) const
  { return modules[mapping.module_global].params[mapping.param_local]; } 

  // TODO and maybe this
  param_desc const& param_at_index(int index) const 
  { return param_at(mappings.params[index]); }
  param_desc const& param_at_tag(int tag) const 
  { return param_at_index(mappings.tag_to_index.at(tag)); }

  plain_value raw_to_plain_at_tag(int tag, double raw) const
  { return raw_to_plain_at_index(mappings.tag_to_index.at(tag), raw); }
  double plain_to_raw_at_tag(int tag, plain_value plain) const
  { return plain_to_raw_at_index(mappings.tag_to_index.at(tag), plain); }
  plain_value raw_to_plain_at_index(int index, double raw) const
  { return param_at(mappings.params[index]).param->domain.raw_to_plain(raw); }
  double plain_to_raw_at_index(int index, plain_value plain) const
  { return param_at(mappings.params[index]).param->domain.plain_to_raw(plain); }

  double normalized_to_raw_at_tag(int tag, normalized_value normalized) const
  { return normalized_to_raw_at_index(mappings.tag_to_index.at(tag), normalized); }
  normalized_value raw_to_normalized_at_tag(int tag, double raw) const
  { return raw_to_normalized_at_index(mappings.tag_to_index.at(tag), raw); }
  double normalized_to_raw_at_index(int index, normalized_value normalized) const
  { return param_at(mappings.params[index]).param->domain.normalized_to_raw(normalized); }
  normalized_value raw_to_normalized_at_index(int index, double raw) const
  { return param_at(mappings.params[index]).param->domain.raw_to_normalized(raw); }

  plain_value normalized_to_plain_at_tag(int tag, normalized_value normalized) const
  { return normalized_to_plain_at_index(mappings.tag_to_index.at(tag), normalized); }
  normalized_value plain_to_normalized_at_tag(int tag, plain_value plain) const
  { return plain_to_normalized_at_index(mappings.tag_to_index.at(tag), plain); }
  plain_value normalized_to_plain_at_index(int index, normalized_value normalized) const
  { return param_at(mappings.params[index]).param->domain.normalized_to_plain(normalized); }
  normalized_value plain_to_normalized_at_index(int index, plain_value plain) const
  { return param_at(mappings.params[index]).param->domain.plain_to_normalized(plain); }
};

}
