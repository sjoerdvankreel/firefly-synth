#pragma once

#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/section.hpp>
#include <plugin_base/shared/utility.hpp>

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace plugin_base {

struct plugin_topo;
enum class module_output { none, cv, audio };
enum class module_stage { input, voice, output };

class module_engine;
typedef std::function<std::unique_ptr<module_engine>(
  plugin_topo const& topo, int sample_rate, int max_frame_count)> 
module_engine_factory;

// module ui
struct module_topo_gui final {
  int section;
  gui_position position;
  gui_dimension dimension;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_topo_gui);
};

// module dsp
struct module_dsp final {
  int output_count;
  int scratch_count;
  module_stage stage;
  module_output output;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_dsp);
};

// module in plugin
struct module_topo final {
  module_dsp dsp;
  topo_info info;
  module_topo_gui gui;

  std::vector<param_topo> params;
  std::vector<param_section> sections;
  module_engine_factory engine_factory;

  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_topo);
  void validate(plugin_topo const& plugin, int index) const;
};

}
