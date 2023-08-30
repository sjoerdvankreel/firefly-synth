#ifndef INFERNAL_BASE_PLUGIN_TOPO_HPP
#define INFERNAL_BASE_PLUGIN_TOPO_HPP

#include <infernal.base/plugin_block.hpp>

#include <vector>
#include <string>

namespace infernal::base {

enum class plugin_kind { synth, fx };
enum class module_scope { voice, global };
enum class module_output { none, cv, audio };

enum class param_slope { linear, log };
enum class param_format { real, step };
enum class param_storage { num, text };
enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_display { toggle, knob, slider };

struct param_topo {
  int stepped_min;
  int stepped_max;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_;
  param_rate rate;
  param_slope slope;
  param_format format;
  param_storage storage;
  param_display display;
  param_direction direction;

  param_value default_value() const;
  std::string to_text(param_value value) const;
  bool from_text(std::string const& text, param_value& value) const;
};

struct submodule_topo {
  int type;
  std::string name;
  std::vector<param_topo> params;
};

struct module_dependency {
  int module_type;
  int module_index;
};

struct module_topo {
  int type;
  int count;
  std::string id;
  std::string name;
  module_scope scope;
  module_output output;
  module_process process;
  std::vector<submodule_topo> submodules;
  std::vector<module_dependency> dependencies;
};

struct plugin_topo {
  bool is_fx;
  plugin_kind kind;
  int polyphony;
  int note_limit;
  int channel_count;
  int block_automation_limit;
  int accurate_automation_limit;
  std::vector<module_topo> modules;
};

struct runtime_param_topo
{
  int id_hash;
  std::string id;
  std::string name;
  int module_index;
  int module_param_index;
  int plugin_param_index;
  param_topo static_topo;
};

struct runtime_module_topo
{
  std::string name;
  module_topo static_topo;
  std::vector<runtime_param_topo> params;
};

struct runtime_plugin_topo
{
  std::vector<runtime_param_topo> runtime_params = {};
  std::vector<runtime_module_topo> runtime_modules = {};
  explicit runtime_plugin_topo(plugin_topo const& topo);
};

}
#endif