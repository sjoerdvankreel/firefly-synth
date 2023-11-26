#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/topo/module.hpp>
#include <plugin_base/shared/state.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

class graph:
public juce::Component
{
  lnf* const _lnf;
  std::vector<float> _data;

public:
  graph(lnf* lnf): _lnf(lnf) {}
  void render(graph_data const& data);
  void paint(juce::Graphics& g) override;
};

class module_graph:
public graph,
public any_state_listener
{
  int const _module_slot;
  int const _module_index;
  int _tweaked_param = -1;
  bool _render_dirty = true;
  plugin_state const* const _state;

  void render_if_dirty();

public:
  ~module_graph() { _state->remove_any_listener(this); }
  module_graph(plugin_state const* state, lnf* lnf, int module_index, int module_slot);

  void paint(juce::Graphics& g) override;
  void any_state_changed(int param, plain_value plain) override;
};

}