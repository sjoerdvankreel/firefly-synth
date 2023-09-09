#pragma once
#include <plugin_base/desc.hpp>
#include <plugin_base/utility.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

class plugin_gui:
public juce::Component
{
  plugin_desc const _desc;
public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  explicit plugin_gui(plugin_topo_factory factory);
};

}