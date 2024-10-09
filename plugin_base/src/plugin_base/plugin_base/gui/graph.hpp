#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/graph_data.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

struct graph_params
{
  enum partition_scale_type { scale_w, scale_h };
  float partition_scale = {};
  std::string name_in_theme = {};
  partition_scale_type scale_type = {};
};

struct module_graph_params
{
  int fps = -1;
  int module_index = -1;
  bool render_on_tweak = false;
  bool render_on_tab_change = false;
  bool render_on_module_mouse_enter = false;
  bool hover_selects_different_graph = false; // for cv matrices
  float fixed_graph_mod_indicator_pos = -1.0f; // for cv matrices
  std::vector<int> render_on_param_mouse_enter_modules = {};
  // trigger also on changes in these
  std::vector<int> dependent_module_indices = {};
}; 

class graph:
public juce::Component
{
private:
  lnf* const _lnf;
  graph_params const _params;
  
  void paint_series(
    juce::Graphics& g, jarray<float, 1> const& series, 
    bool bipolar, float stroke_thickness, float midpoint);

  void paint_multi_bars(
    juce::Graphics& g, int implied_count,
    std::vector<std::pair<int, float>> const& multi_bars);

protected:

  graph_data _data;
  plugin_gui* const _gui;

  // horizontal positions for lfo/env
  double _mod_indicators_activated = {};
  std::vector<float> _mod_indicators = {};

  // anything really, but currently used for free-running or continuous-update-seed random lfos
  double _custom_outputs_activated = {};
  std::vector<mod_out_custom_state> _custom_outputs = {};
  
  // its probably better to merge graph and module_graph
  // cant think of anything that is a "plain" graph
  virtual float fixed_graph_mod_indicator_pos() const { return -1.0f; }

public:
  void render(graph_data const& data);
  void paint(juce::Graphics& g) override;

  graph(plugin_gui* gui, lnf* lnf, graph_params const& params);
};

// taps into module_topo.graph_renderer based on task tweaked/hovered param
class module_graph:
public graph,
public any_state_listener,
public gui_mouse_listener,
public gui_tab_selection_listener,
public modulation_output_listener,
public juce::Timer,
public juce::SettableTooltipClient
{
  bool _done = false;
  bool _render_dirty = true;
  int _activated_module_slot = 0;
  int _hovered_param = -1;
  int _hovered_or_tweaked_param = -1;
  int _last_rerender_cause_param = -1;

  module_graph_params const _module_params;

  bool render_if_dirty();
  void request_rerender(int param, bool hover);

protected:
  float fixed_graph_mod_indicator_pos() const override
  { return _module_params.fixed_graph_mod_indicator_pos; }

public:
  ~module_graph();
  module_graph(
    plugin_gui* gui, lnf* lnf, graph_params const& params, 
    module_graph_params const& module_params);

  void timerCallback() override;
  void paint(juce::Graphics& g) override;

  void param_mouse_enter(int param) override;
  void module_mouse_enter(int module) override;
  void module_tab_changed(int module, int slot) override;
  void any_state_changed(int param, plain_value plain) override;  

  void modulation_outputs_reset() override;
  void modulation_outputs_changed(std::vector<modulation_output> const& outputs) override;
};

}