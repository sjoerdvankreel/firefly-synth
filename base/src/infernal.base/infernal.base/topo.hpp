#pragma once
#include <infernal.base/utility.hpp>
#include <vector>
#include <string>
#include <memory>

namespace infernal::base {

class module_engine;
class mixdown_engine;

enum class plugin_type { synth, fx };
enum class module_scope { voice, global };
enum class module_output { none, cv, audio };

enum class param_storage { item, num };
enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_format { step, linear, log };
enum class param_display { toggle, list, knob, hslider, vslider };

struct item_topo final {
  std::string id;
  std::string name;
  item_topo(std::string const& id, std::string const& name): id(id), name(name) {}
  INF_DECLARE_MOVE_ONLY(item_topo);
};

struct param_topo final {
  int group;
  double min;
  double max;
  bool percentage;
  param_rate rate;
  param_format format;
  param_storage storage;
  param_display display;
  param_direction direction;
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

struct module_group_topo final {
  int module_count;
  std::string id;
  std::string name;
  module_scope scope;
  module_output output;
  std::vector<param_topo> params;
  std::vector<param_group_topo> param_groups;
  std::unique_ptr<module_engine>(*engine_factory)(int sample_rate, int max_frame_count);
  INF_DECLARE_MOVE_ONLY(module_group_topo);
};

struct plugin_topo final {
  int polyphony;
  plugin_type type;
  std::vector<module_group_topo> module_groups;
  std::unique_ptr<mixdown_engine>(*mixdown_factory)(int sample_rate, int max_frame_count);
  INF_DECLARE_MOVE_ONLY(plugin_topo);
};

}
