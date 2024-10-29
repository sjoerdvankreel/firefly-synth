#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

struct mseg_seg
{
  float x;
  float y;
  float slope;
};

// visual mseg editor normalized in xy [0, 0], [1, 1]
// the plug must provide:
// 1 start-y and 1 end-y param
// counted X, Y, On parameters (at least 1 so section count = N + 1)
// counted slope parameters with count = N + 1
// optional 1 sustain point in case of envelope
// we keep a local copy of all param values and
// just flush the entire thing to the plug param state
// on each change -- this simplifies splicing / joining segs
// and also helps to keep stuff sorted
// TODO replace on_param by seg_count and keep stuff in check
class mseg_editor:
public juce::Component,
public juce::DragAndDropContainer,
public juce::DragAndDropTarget,
public juce::SettableTooltipClient,
public state_listener
{
  plugin_gui* const _gui;
  lnf* const _lnf;

  int const _module_index;
  int const _module_slot;
  /*
  int const _start_y_param;
  int const _end_y_param;
  int const _on_param;
  int const _x_param;
  int const _y_param;
  int const _slope_param;
  TODO */

  // index into gui copy of params
  /*
  int _hit_test_point = -1;
  int _hit_test_slope = -1;
  bool _hit_test_end_y = false;
  bool _hit_test_start_y = false;
  int _dragging_point = -1;
  int _dragging_slope = -1;
  bool _dragging_end_y = false;
  bool _dragging_start_y = false;
  TODO */

  float _gui_start_y = 0.0f;
  std::vector<mseg_seg> _gui_segs = {};

  bool hit_test(juce::MouseEvent const& e);

  float sloped_y_pos(
    float pos, int seg) const;

  void make_slope_path(
    float x, float y, float w, float h, 
    int seg, bool closed, juce::Path& path) const;

public:
  
  void paint(juce::Graphics& g) override;
  void state_changed(int index, plain_value plain) override;

  void mouseDown(juce::MouseEvent const& event) override;
  void mouseDrag(juce::MouseEvent const& event) override;
  void mouseMove(juce::MouseEvent const& event) override;
  void mouseDoubleClick(juce::MouseEvent const& event) override;

  void itemDropped(juce::DragAndDropTarget::SourceDetails const& details) override;
  void itemDragMove(juce::DragAndDropTarget::SourceDetails const& details) override;
  bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& details) override;

  ~mseg_editor();
  mseg_editor(
    plugin_gui* gui, lnf* lnf, int module_index, int module_slot, 
    int start_y_param, int end_y_param, int on_param, int x_param, int y_param, int slope_param);
};

}