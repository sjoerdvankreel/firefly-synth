#pragma once
#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <vector>
#include <string>
#include <cassert>

namespace infernal::base {

struct plugin_topo;
struct plugin_block;

enum class plugin_kind { synth, fx };
enum class module_scope { voice, global };
enum class module_output { none, cv, audio };

enum class param_storage { num, text };
enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_format { step, linear, log };
enum class param_display { toggle, list, knob, slider };

typedef void(*module_process)(
plugin_topo const& topo, int module_index, plugin_block const& block); 

struct plugin_dimensions final {
  std::vector<int> module_counts;
  std::vector<std::vector<int>> module_frame_counts;
  std::vector<std::vector<int>> module_param_counts;
  std::vector<std::vector<int>> module_channel_counts;
  std::vector<std::vector<std::vector<int>>> module_param_frame_counts;
  std::vector<std::vector<std::vector<int>>> module_channel_frame_counts;
  INF_DECLARE_MOVE_ONLY(plugin_dimensions);
}; 

struct param_topo final {
  int type;
  int precision;
  double min;
  double max;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_text;
  param_rate rate;
  param_format format;
  param_storage storage;
  param_display display;
  param_direction direction;
  INF_DECLARE_MOVE_ONLY(param_topo);

  param_value default_value() const;
  std::string to_text(param_value value) const;
  double to_normalized(param_value value) const;
  param_value from_normalized(double normalized) const;
  bool from_text(std::string const& text, param_value& value) const;
};

struct param_group_topo final {
  std::string name;
  std::vector<int> param_types;
  INF_DECLARE_MOVE_ONLY(param_group_topo);
};

struct module_callbacks final {
  module_process process;
};

struct module_group_topo final {
  int type;
  int module_count;
  std::string id;
  std::string name;
  module_scope scope;
  module_output output;
  module_callbacks callbacks;
  std::vector<param_topo> params;
  std::vector<param_group_topo> param_groups;
  INF_DECLARE_MOVE_ONLY(module_group_topo);
};

struct plugin_topo final {
  bool is_fx;
  plugin_kind kind;
  int polyphony;
  int note_limit;
  int channel_count;
  int block_automation_limit;
  int accurate_automation_limit;
  std::vector<module_group_topo> module_groups;
  plugin_dimensions dimensions(int frame_count);
  INF_DECLARE_MOVE_ONLY(plugin_topo);
};

struct param_mapping final {
  int module_group = {};
  int module_index = {};
  int param_index = {};
};

struct param_desc final {
  int id_hash = {};
  std::string id = {};
  std::string name = {};

  INF_DECLARE_MOVE_ONLY(param_desc);
  param_desc(module_group_topo const& module_group, int module_index, param_topo const& param);
};

struct module_desc final {
  std::string name = {};
  std::vector<param_desc> params = {};
  
  INF_DECLARE_MOVE_ONLY(module_desc);
  module_desc(module_group_topo const& module_group, int module_index);
};

struct plugin_desc final {
  std::vector<module_desc> modules = {};
  std::vector<param_mapping> param_mappings = {};

  INF_DECLARE_MOVE_ONLY(plugin_desc);
  plugin_desc(plugin_topo const& plugin);
};

inline double 
param_topo::to_normalized(param_value value) const
{
  switch (format)
  {
  case param_format::log:
  case param_format::linear: return (value.real - min) / (max - min);
  case param_format::step: return (value.step - min) / (max - min);
  default: assert(false); return 0;
  }
}

inline param_value 
param_topo::from_normalized(double normalized) const
{
  switch (format)
  {
  case param_format::log:
  case param_format::linear: return param_value::from_real(min + normalized * (max - min));
  case param_format::step: return param_value::from_step(min + normalized * (max - min));
  default: assert(false); return {};
  }
}

}
#pragma once