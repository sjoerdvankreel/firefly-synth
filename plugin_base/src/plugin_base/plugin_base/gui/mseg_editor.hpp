#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

struct mseg_seg
{
  float w;
  float y;
  float slope;
};

// visual mseg editor normalized in xy [0, 0], [1, 1]
// the plug must provide:
// 1 start-y and count param
// 1 grid_x and 1 grid_y param for snapping
// counted Width, Y, Slope parameters (at least 1 so section count = N)
// optional 1 sustain point in case of envelope
// we keep a local copy of all param values and
// just flush the entire thing to the plug param state
// on change -- this simplifies splicing / joining segs
// and also helps to keep stuff sorted
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
  int const _sustain_param;
  int const _w_param;
  int const _y_param;
  int const _slope_param;
  int const _snap_x_param;
  int const _snap_y_param;

  bool const _is_external;

  // dont get picked up from the main window, so needed for external
  std::unique_ptr<juce::TooltipWindow> _tooltip = {};

  int _drag_seg = -1;
  bool _drag_start_y = false;
  bool _drag_seg_slope = false;
  
  float _drag_seg_initial_w = 0.0f;
  float _drag_seg_initial_x = 0.0f;

  bool _is_dirty = false;
  int _undo_token = -1;
  int _max_seg_count = -1;
  int _current_seg_count = -1;
  float _gui_start_y = 0.0f;
  std::vector<mseg_seg> _gui_segs = {};

  void init_from_plug_state();

  float sloped_y_pos(
    float pos, int seg) const;
  float get_seg_total_x(
    int seg) const;

  float get_seg_last_total_x() const 
  { return get_seg_total_x(_gui_segs.size() - 1); }
  float get_seg_norm_x(int seg) const
  { return get_seg_total_x(seg) / get_seg_last_total_x(); }

  bool hit_test(
    juce::MouseEvent const& e, bool& hit_start_y,
    int& hit_seg, bool& hit_seg_slope) const;
  void make_slope_path(
    float x, float y, float w, float h, 
    int seg, bool closed, juce::Path& path) const;

public:
  
  void paint(juce::Graphics& g) override;
  void state_changed(int index, plain_value plain) override;

  void mouseUp(juce::MouseEvent const& event) override;
  void mouseDrag(juce::MouseEvent const& event) override;
  void mouseMove(juce::MouseEvent const& event) override;
  void mouseDoubleClick(juce::MouseEvent const& event) override;

  void itemDropped(juce::DragAndDropTarget::SourceDetails const& details) override {};
  void itemDragMove(juce::DragAndDropTarget::SourceDetails const& details) override;
  bool isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& details) override;

  ~mseg_editor();
  mseg_editor(
    plugin_gui* gui, lnf* lnf, int module_index, int module_slot, 
    int start_y_param, int count_param, int sustain_param, int w_param, int y_param, 
    int slope_param, int snap_x_param, int snap_y_param, bool is_external);
};

}