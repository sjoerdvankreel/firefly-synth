#pragma once
#include <infernal.base/utility.hpp>
#include <vector>
#include <string>

namespace infernal::base {

struct plugin_topo;
struct plugin_block;

enum class plugin_kind { synth, fx };
enum class module_scope { voice, global };
enum class module_output { none, cv, audio };

enum class param_storage { num, list };
enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_format { step, linear, log };
enum class param_display { toggle, list, knob, hslider, vslider };

typedef void(*module_process)(
plugin_topo const& topo, int module_index, plugin_block const& block);

struct item_topo final {
  std::string id;
  std::string name;
  item_topo(std::string const& id, std::string const& name): id(id), name(name) {}
  INF_DECLARE_MOVE_ONLY(item_topo);
};

struct param_config final {
  param_rate rate;
  param_format format;
  param_storage storage;
  param_display display;
  param_direction direction;
};

struct param_topo final {
  int group;
  double min;
  double max;
  bool percentage;
  param_config config;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_text;
  std::vector<item_topo> list;
  INF_DECLARE_MOVE_ONLY(param_topo);
};

struct param_group_topo final {
  int type;
  std::string name;
  param_group_topo(int type, std::string const& name): type(type), name(name) {}
  INF_DECLARE_MOVE_ONLY(param_group_topo);
};

struct module_callbacks final {
  module_process process;
};

struct module_group_topo final {
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
  int block_automation_limit;
  int accurate_automation_limit;
  std::vector<module_group_topo> module_groups;
  INF_DECLARE_MOVE_ONLY(plugin_topo);
};

}
