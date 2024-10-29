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
  int const _start_y_param;
  int const _count_param;
  int const _x_param;
  int const _y_param;
  int const _slope_param;

  int _drag_seg = -1;
  bool _drag_start_y = false;
  bool _drag_seg_slope = false;

  int _max_seg_count = -1;
  int _current_seg_count = -1;
  float _gui_start_y = 0.0f;
  std::vector<mseg_seg> _gui_segs = {};

  void init_from_plug_state();

  float sloped_y_pos(
    float pos, int seg) const;
  bool hit_test(
    juce::MouseEvent const& e, bool& hit_start_y,
    int& hit_seg, bool& hit_seg_slope) const;
  void make_slope_path(
    float x, float y, float w, float h, 
    int seg, bool closed, juce::Path& path) const;

public:
  
  void paint(juce::Graphics& g) override;
  void state_changed(int index, plain_value plain) override;

  void mouseDrag(juce::MouseEvent const& event) override;
  void mouseMove(juce::MouseEvent const& event) override;
  void mouseDoubleClick(juce::MouseEvent const& event) override;

  void itemDropped(juce::DragAndDropTarget::SourceDetails const& details) override;
  void itemDragMove(juce::DragAndDropTarget::SourceDetails const& details) override;
  bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& details) override;

  ~mseg_editor();
  mseg_editor(
    plugin_gui* gui, lnf* lnf, int module_index, int module_slot, 
    int start_y_param, int count_param, int x_param, int y_param, int slope_param);
};

}