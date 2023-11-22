#pragma once

#include <plugin_base/desc/param.hpp>
#include <plugin_base/desc/module.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/shared/value.hpp>
#include <plugin_base/shared/jarray.hpp>
#include <plugin_base/shared/utility.hpp>

#include <map>
#include <set>
#include <vector>
#include <string>
#include <filesystem>

namespace plugin_base {

struct plugin_desc;

// from resources folder
struct factory_preset
{
  std::string name;
  std::string path;
};

// differences between plugin formats
struct format_config {
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(format_config);
  virtual std::filesystem::path 
  resources_folder(std::filesystem::path const& binary_path) const = 0;
};

// mapping plugin level midi source index
struct midi_mapping final {
  int midi_local = {};
  int midi_global = {};
  int module_global = {};
  midi_topo_mapping topo = {};
  std::vector<int> linked_params = {};
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(midi_mapping);
};

// mapping plugin level parameter index
struct param_mapping final {
  int param_local = {};
  int param_global = {};
  int module_global = {};
  int midi_source_global = -1; // for midi linked params
  param_topo_mapping topo = {};
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(param_mapping);
};

// mapping to/from global midi source info
struct plugin_midi_mappings final {
  std::vector<int> index_to_tag = {};
  std::map<int, int> id_to_index = {};
  std::map<int, int> tag_to_index = {};
  std::vector<midi_mapping> midi_sources = {};
  std::vector<std::vector<std::vector<int>>> topo_to_index = {};

  void validate(plugin_desc const& plugin) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_midi_mappings);
};

// mapping to/from global parameter info
struct plugin_param_mappings final {
  std::vector<int> index_to_tag = {};
  std::map<int, int> tag_to_index = {};
  std::vector<param_mapping> params = {};
  std::map<std::string, std::map<std::string, int>> id_to_index = {};
  std::vector<std::vector<std::vector<std::vector<int>>>> topo_to_index = {};

  void validate(plugin_desc const& plugin) const;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_param_mappings);
};

// runtime plugin descriptor
struct plugin_desc final {
private:
  param_desc const& param_at_mapping(param_mapping const& mapping) const
  { return modules[mapping.module_global].params[mapping.param_local]; } 
  midi_desc const& midi_at_mapping(midi_mapping const& mapping) const
  { return modules[mapping.module_global].midi_sources[mapping.midi_local]; } 

public:
  int midi_count = {};
  int param_count = {};
  int module_count = {};
  int module_voice_start = {};
  int module_output_start = {};

  plugin_topo const* plugin = {};
  format_config const* config = {};
  std::vector<module_desc> modules = {};
  plugin_midi_mappings midi_mappings = {};
  plugin_param_mappings param_mappings = {};
  std::vector<param_desc const*> params = {};
  std::vector<midi_desc const*> midi_sources = {};
  std::map<std::string, int> module_id_to_index = {};

  void validate() const;
  std::vector<factory_preset> presets() const;

  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(plugin_desc);
  plugin_desc(plugin_topo const* plugin, format_config const* config);

  param_desc const& param_at_index(int index) const 
  { return param_at_mapping(param_mappings.params[index]); }
  param_desc const& param_at_tag(int tag) const
  { return param_at_index(param_mappings.tag_to_index.at(tag)); }
  param_topo const& param_topo_at(int module, int param) const
  { return plugin->modules[module].params[param]; }

  plain_value raw_to_plain_at(int module, int param, double raw) const
  { return param_topo_at(module, param).domain.raw_to_plain(raw); }
  double plain_to_raw_at(int module, int param, plain_value plain) const
  { return param_topo_at(module, param).domain.plain_to_raw(plain); }
  plain_value raw_to_plain_at_tag(int tag, double raw) const
  { return raw_to_plain_at_index(param_mappings.tag_to_index.at(tag), raw); }
  double plain_to_raw_at_tag(int tag, plain_value plain) const
  { return plain_to_raw_at_index(param_mappings.tag_to_index.at(tag), plain); }
  plain_value raw_to_plain_at_index(int index, double raw) const
  { return param_at_mapping(param_mappings.params[index]).param->domain.raw_to_plain(raw); }
  double plain_to_raw_at_index(int index, plain_value plain) const
  { return param_at_mapping(param_mappings.params[index]).param->domain.plain_to_raw(plain); }

  double normalized_to_raw_at(int module, int param, normalized_value normalized) const
  { return param_topo_at(module, param).domain.normalized_to_raw(normalized); }
  normalized_value raw_to_normalized_at(int module, int param, double raw) const
  { return param_topo_at(module, param).domain.raw_to_normalized(raw); }
  double normalized_to_raw_at_tag(int tag, normalized_value normalized) const
  { return normalized_to_raw_at_index(param_mappings.tag_to_index.at(tag), normalized); }
  normalized_value raw_to_normalized_at_tag(int tag, double raw) const
  { return raw_to_normalized_at_index(param_mappings.tag_to_index.at(tag), raw); }
  double normalized_to_raw_at_index(int index, normalized_value normalized) const
  { return param_at_mapping(param_mappings.params[index]).param->domain.normalized_to_raw(normalized); }
  normalized_value raw_to_normalized_at_index(int index, double raw) const
  { return param_at_mapping(param_mappings.params[index]).param->domain.raw_to_normalized(raw); }

  plain_value normalized_to_plain_at(int module, int param, normalized_value normalized) const
  { return param_topo_at(module, param).domain.normalized_to_plain(normalized); }
  normalized_value plain_to_normalized_at(int module, int param, plain_value plain) const
  { return param_topo_at(module, param).domain.plain_to_normalized(plain); }
  plain_value normalized_to_plain_at_tag(int tag, normalized_value normalized) const
  { return normalized_to_plain_at_index(param_mappings.tag_to_index.at(tag), normalized); }
  normalized_value plain_to_normalized_at_tag(int tag, plain_value plain) const
  { return plain_to_normalized_at_index(param_mappings.tag_to_index.at(tag), plain); }
  plain_value normalized_to_plain_at_index(int index, normalized_value normalized) const
  { return param_at_mapping(param_mappings.params[index]).param->domain.normalized_to_plain(normalized); }
  normalized_value plain_to_normalized_at_index(int index, plain_value plain) const
  { return param_at_mapping(param_mappings.params[index]).param->domain.plain_to_normalized(plain); }
};

}
