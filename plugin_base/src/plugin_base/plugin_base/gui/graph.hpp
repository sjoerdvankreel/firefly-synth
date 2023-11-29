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

// full audio engine graph
class audio_graph:
public graph,
public juce::Timer
{
  int const _midi_key;
  float const _duration;
  int const _sample_rate;
  plugin_state const* const _state;

public:
  void paint(juce::Graphics& g) override;
  void timerCallback() override { repaint(); }
  
  ~audio_graph() { stopTimer(); }
  audio_graph(plugin_state const* state, lnf* lnf, int sample_rate, int midi_key, float duration, int fps = 1):
  graph(lnf), _midi_key(midi_key), _duration(duration), _sample_rate(sample_rate), _state(state) { startTimerHz(fps); }
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