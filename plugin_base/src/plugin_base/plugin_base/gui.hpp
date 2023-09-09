#pragma once
#include <plugin_base/desc.hpp>
#include <plugin_base/utility.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

class plugin_gui:
public juce::Component
{
  plugin_desc const _desc;
  void layout();

public:
  INF_DECLARE_MOVE_ONLY(plugin_gui);
  explicit plugin_gui(plugin_topo_factory factory);

  void paint(juce::Graphics& g) override;
  plugin_desc const& desc() const { return _desc; }
};

}