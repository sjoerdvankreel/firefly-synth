#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/state.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class graph:
public juce::Component
{
  lnf* const _lnf;
  std::vector<float> _data;

public:
  graph(lnf* lnf): _lnf(lnf) {}
  void render(graph_data const& data);
  void paint(juce::Graphics& g) override;
};

}