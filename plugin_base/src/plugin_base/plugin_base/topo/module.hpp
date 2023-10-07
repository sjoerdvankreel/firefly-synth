#pragma once

#include <plugin_base/utility.hpp>
#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/section.hpp>

#include <memory>
#include <vector>
#include <string>

namespace plugin_base {

struct plugin_topo;
enum class module_output { none, cv, audio };
enum class module_stage { input, voice, output };

class module_engine;
typedef std::unique_ptr<module_engine>(*
module_engine_factory)(
  int slot, int sample_rate, int max_frame_count);

// module ui
struct module_topo_gui final {
  gui_layout layout;
  gui_position position;
  gui_dimension dimension;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(module_topo_gui);
};

// module dsp
struct module_dsp final {
  int output_count;
  module_stage stage;
  module_output output;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(module_dsp);
};

// module in plugin
struct module_topo final {
  module_dsp dsp;
  topo_info info;
  module_topo_gui gui;

  std::vector<param_topo> params;
  std::vector<section_topo> sections;
  module_engine_factory engine_factory;

  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(module_topo);
  void validate(plugin_topo const& plugin, int index) const;
};

}
