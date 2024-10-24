#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class mseg_editor:
public juce::Component
{
public:
  void paint(juce::Graphics& g) override;
};

}