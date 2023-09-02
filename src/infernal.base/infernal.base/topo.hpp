#pragma once
#include <infernal.base/utility.hpp>
#include <infernal.base/param_value.hpp>
#include <vector>
#include <string>

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
  
struct param_topo final {
  int type;
  double min;
  double max;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_;
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

struct group_topo final {
  std::string name;
  std::vector<int> param_types;
  INF_DECLARE_MOVE_ONLY(group_topo);
};

struct module_callbacks final {
  module_process process;
};

struct module_topo final {
  int type;
  int count;
  std::string id;
  std::string name;
  module_scope scope;
  module_output output;
  module_callbacks callbacks;
  std::vector<group_topo> groups;
  std::vector<param_topo> params;
  INF_DECLARE_MOVE_ONLY(module_topo);
};

struct plugin_topo final {
  bool is_fx;
  plugin_kind kind;
  int polyphony;
  int note_limit;
  int channel_count;
  int block_automation_limit;
  int accurate_automation_limit;
  std::vector<module_topo> modules;
  INF_DECLARE_MOVE_ONLY(plugin_topo);
};

struct param_mapping final {
  int module_type;
  int module_index;
  int param_type;
};

struct param_desc final {
  int id_hash;
  std::string id;
  std::string name;

  INF_DECLARE_MOVE_ONLY(param_desc);
  param_desc(module_topo const& module_topo, int module_index, param_topo const& param_topo);
};

struct module_desc final {
  std::string name;
  std::vector<param_desc> params;
  
  INF_DECLARE_MOVE_ONLY(module_desc);
  module_desc(module_topo const& topo, int module_index);
};

struct plugin_desc final {
  std::vector<param_desc> params = {};
  std::vector<module_desc> modules = {};
  std::vector<param_mapping> mapping = {};

  INF_DECLARE_MOVE_ONLY(plugin_desc);
  plugin_desc(plugin_topo const& topo);
};

}
#pragma once