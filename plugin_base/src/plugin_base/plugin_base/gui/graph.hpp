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
  graph_data _data = {};  
  void paint_series(juce::Graphics& g, jarray<float, 1> const& series);

public:
  graph(lnf* lnf): _lnf(lnf) {}
  void render(graph_data const& data);
  void paint(juce::Graphics& g) override;
};

struct module_graph_params
{
  int fps = -1;
  bool render_on_tweak = false;
  bool render_on_hover = false;
};

// taps into module_topo.graph_renderer based on task tweaked/hovered param
class module_graph:
public graph,
public juce::Timer,
public gui_listener, 
public any_state_listener
{
  bool _done = false;
  plugin_gui* const _gui;
  bool _render_dirty = true;
  module_graph_params const _params;
  int _hovered_or_tweaked_param = -1;

  void render_if_dirty();
  void request_rerender(int param);

public:
  ~module_graph();
  module_graph(plugin_gui* gui, lnf* lnf, module_graph_params const& params);

  void timerCallback() override;
  void paint(juce::Graphics& g) override;
  void param_mouse_enter(int param) override;
  void module_mouse_enter(int module) override;

  void module_mouse_exit(int module) override { render(graph_data()); }
  void any_state_changed(int param, plain_value plain) override { request_rerender(param); }
};

}