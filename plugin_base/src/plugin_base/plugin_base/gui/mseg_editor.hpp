#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

// visual mseg editor normalized in xy [0, 0], [1, 1]
// the plug must provide:
// 1 start-y and 1 end-y param
// counted X, Y, On parameters (at least 1 so N = section count >= 2)
// counted slope parameters with count = N - 1
// optional 1 sustain point in case of envelope
class mseg_editor:
public juce::Component
{
  plugin_gui* const _gui;
  int const _module_index;
  int const _module_slot;
  int const _start_y_param;
  int const _end_y_param;
public:
  void paint(juce::Graphics& g) override;
  mseg_editor(plugin_gui* gui, int module_index, int module_slot, int start_y_param, int end_y_param);
};

}