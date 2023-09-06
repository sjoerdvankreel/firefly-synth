#pragma once
#include <plugin_base/utility.hpp>
#include <vector>
#include <string>
#include <memory>

namespace infernal::base {

class module_engine;
class mixdown_engine;

enum class plugin_type { synth, fx };
enum class module_scope { voice, global };
enum class module_output { none, cv, audio };

enum class param_rate { accurate, block };
enum class param_direction { input, output };
enum class param_type { step, name, item, linear, log };
enum class param_display { toggle, list, knob, hslider, vslider };

// item within list (persisted by guid)
struct item_topo final {
  std::string id;
  std::string name;
  item_topo(std::string const& id, std::string const& name): id(id), name(name) {}
  INF_DECLARE_MOVE_ONLY(item_topo);
};

// param within module
struct param_topo final {
  int group;
  double min;
  double max;
  double exp;
  bool percentage;
  param_type type;
  param_rate rate;
  param_display display;
  param_direction direction;
  std::string id;
  std::string name;
  std::string unit;
  std::string default_text;
  std::vector<item_topo> items;
  std::vector<std::string> names;
  INF_DECLARE_MOVE_ONLY(param_topo);
  bool is_real() const { return type == param_type::log || type == param_type::linear; }
};

// grouping parameters for gui
struct param_group_topo final {
  int type;
  std::string name;
  param_group_topo(int type, std::string const& name): type(type), name(name) {}
  INF_DECLARE_MOVE_ONLY(param_group_topo);
};

// module group within plugin (may be more than 1 per group)
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

// plugin definition
struct plugin_topo final {
  int polyphony;
  plugin_type type;
  std::vector<module_group_topo> module_groups;
  std::unique_ptr<mixdown_engine>(*mixdown_factory)(int sample_rate, int max_frame_count);
  INF_DECLARE_MOVE_ONLY(plugin_topo);
};

}
