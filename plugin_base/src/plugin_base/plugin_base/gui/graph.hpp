#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/gui.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/state.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class graph:
public juce::Component
{
  lnf* const _lnf;
  graph_data _data = {};

public:
  graph(lnf* lnf): _lnf(lnf) {}
  void render(graph_data const& data);
  void paint(juce::Graphics& g) override;
};

// taps into module_topo.graph_renderer
// set both slot and index to -1 to respond to last tweaked parameter
class module_graph:
public graph,
public juce::Timer,
public gui_listener,
public any_state_listener
{
  bool _done = false;
  plugin_gui* const _gui;
  bool const _any_module;
  int const _module_slot;
  int const _module_index;
  int _tweaked_param = -1;
  bool _render_dirty = true;

  void mouse_exit();
  void render_if_dirty();

public:
  void param_mouse_enter(int module) override;
  void module_mouse_enter(int module) override;
  void param_mouse_exit(int param) override { mouse_exit(); }
  void module_mouse_exit(int module) override { mouse_exit(); }

  void timerCallback() override;
  void paint(juce::Graphics& g) override;
  void any_state_changed(int param, plain_value plain) override;

  ~module_graph();
  module_graph(plugin_gui* gui, lnf* lnf, int module_index, int module_slot, int fps = 10);
};

}