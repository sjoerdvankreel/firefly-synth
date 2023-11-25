#pragma once

#include <plugin_base/gui/lnf.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class graph:
public juce::Component
{
  bool _bipolar;
  lnf* const _lnf;
  std::vector<float> _data;

public:
  graph(lnf* lnf): _lnf(lnf) {}
  void paint(juce::Graphics& g) override;
  void render(std::vector<float> const& data, bool bipolar);
};

}