#pragma once

#include <plugin_base/topo/param.hpp>
#include <plugin_base/topo/shared.hpp>
#include <plugin_base/topo/section.hpp>
#include <plugin_base/shared/utility.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

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

// module color scheme
struct module_gui_colors final {
  juce::Colour tab_text = juce::Colour(0xFFFF8844);
  juce::Colour tab_button = juce::Colour(0xFF333333);
  juce::Colour tab_header = juce::Colour(0xFF222222);
  juce::Colour tab_background1 = juce::Colour(0xFF222222);
  juce::Colour tab_background2 = juce::Colour(0xFF111111);
  juce::Colour dropdown_text = juce::Colour(0xFFFF0000);
  juce::Colour dropdown_arrow = juce::Colour(0xFF00FFFF);
  juce::Colour dropdown_outline = juce::Colour(0xFF0000FF);
  juce::Colour dropdown_background = juce::Colour(0xFF00FF00);
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(module_gui_colors);
};

// module ui
struct module_topo_gui final {
  int section;
  gui_position position;
  gui_dimension dimension;
  module_gui_colors colors;
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
