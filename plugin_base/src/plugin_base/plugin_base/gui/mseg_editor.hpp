#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

// visual mseg editor normalized in xy [0, 0], [1, 1]
// the plug must provide:
// 1 start-y and 1 end-y param
// counted X, Y, On parameters (at least 1 so section count = N + 1)
// counted slope parameters with count = N + 1
// optional 1 sustain point in case of envelope
class mseg_editor:
public juce::Component
{
  plugin_gui* const _gui;
  lnf* const _lnf;

  int const _module_index;
  int const _module_slot;
  int const _start_y_param;
  int const _end_y_param;
  int const _on_param;
  int const _x_param;
  int const _y_param;
  int const _slope_param;

  int _hovered_point = -1;
  int _hovered_slope = -1;
  bool _hovered_end_y = false;
  bool _hovered_start_y = false;

  float sloped_y_pos(
    float pos, int index, 
    float y1, float y2) const;

  void make_slope_path(
    float x, float y, float w, float h,
    std::pair<float, float> const& from, 
    std::pair<float, float> const& to, 
    int slope_index, bool closed, juce::Path& path) const;

public:
  void paint(juce::Graphics& g) override;
  void mouseMove(juce::MouseEvent const& event) override;

  mseg_editor(
    plugin_gui* gui, lnf* lnf, int module_index, int module_slot, 
    int start_y_param, int end_y_param, int on_param, int x_param, int y_param, int slope_param);
};

}