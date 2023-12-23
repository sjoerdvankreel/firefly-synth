#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/shared/graph_data.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class graph:
public juce::Component
{
  lnf* const _lnf;
  graph_data _data;  
  void paint_series(juce::Graphics& g, jarray<float, 1> const& series);

public:
  void render(graph_data const& data);
  void paint(juce::Graphics& g) override;
  graph(lnf* lnf) : _lnf(lnf), _data(graph_data_type::na, {}) {}
};

struct module_graph_params
{
  int fps = -1;
  int module_index = -1;
  bool render_on_tweak = false;
  bool render_on_tab_change = false;
  bool render_on_module_mouse_enter = false;
  std::vector<int> render_on_param_mouse_enter_modules = {};
  // trigger also on changes in these
  std::vector<int> dependent_module_indices = {};
};

// taps into module_topo.graph_renderer based on task tweaked/hovered param
class module_graph:
public graph,
public any_state_listener,
public gui_mouse_listener,
public gui_tab_selection_listener,
public juce::Timer,
public juce::SettableTooltipClient
{
  plugin_gui* const _gui;
  module_graph_params const _params;

  bool _done = false;
  bool _render_dirty = true;
  int _activated_module_slot = 0;
  int _hovered_or_tweaked_param = -1;
  int _last_rerender_cause_param = -1;

  bool render_if_dirty();
  void request_rerender(int param);

public:
  ~module_graph();
  module_graph(plugin_gui* gui, lnf* lnf, module_graph_params const& params);

  void timerCallback() override;
  void paint(juce::Graphics& g) override;

  void param_mouse_enter(int param) override;
  void module_mouse_enter(int module) override;
  void module_tab_changed(int module, int slot) override;
  void any_state_changed(int param, plain_value plain) override;  
};

}