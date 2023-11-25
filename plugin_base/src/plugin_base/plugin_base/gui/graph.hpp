#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/shared/state.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

namespace plugin_base {

// renders pretty images
class graph:
public juce::Component
{
  lnf* const _lnf;
  std::vector<float> _data;

public:
  graph(lnf* lnf): _lnf(lnf) {}
  void paint(juce::Graphics& g) override;
  void paint(std::vector<float> const& data);
};

// renders pretty images based on static plugin parameter state 
// (i.e. based on ui values only, nothing from the audio thread)
class plugin_graph:
public graph,
public any_state_listener
{
  plugin_state const* const _state;

protected:
  virtual std::vector<float> render() = 0;
  virtual bool should_render(int index) = 0;
  plugin_state const* const state() const { return _state; }

public:
  void any_state_changed(int index, plain_value plain);
  ~plugin_graph() { _state->remove_any_listener(this); }
  plugin_graph(lnf* lnf, plugin_state const* state): graph(lnf), _state(state)
  { _state->add_any_listener(this); }
};

}