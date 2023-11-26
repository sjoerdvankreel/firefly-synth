#pragma once

#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/shared/state.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>

namespace plugin_base {

class graph_source
{
public:
  virtual bool should_render(plugin_desc const& desc, int param) = 0;
  virtual std::vector<float> render(plugin_state const& state, int param) = 0;
};

class graph:
public juce::Component,
public any_state_listener
{
  lnf* const _lnf;
  std::vector<float> _data;
  plugin_state const* const _state;
  std::unique_ptr<graph_source> _source;

public:
  void paint(juce::Graphics& g) override;
  void any_state_changed(int param, plain_value plain) override;
  
  ~graph() { _state->remove_any_listener(this); }
  graph(plugin_state const* state, lnf* lnf, std::unique_ptr<graph_source>&& source): 
  _lnf(lnf), _data(source->render(*state, 0)), _state(state), _source(std::move(source)) 
  { _state->add_any_listener(this); }
};

}