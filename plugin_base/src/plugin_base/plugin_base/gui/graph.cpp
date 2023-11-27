#include <plugin_base/gui/graph.hpp>
#include <plugin_base/dsp/utility.hpp>

using namespace juce;

namespace plugin_base {

module_graph::
~module_graph() 
{ 
  _done = true;
  _state->remove_any_listener(this); 
  stopTimer();
}

module_graph::
module_graph(plugin_state const* state, lnf* lnf, int module_index, int module_slot, int fps):
graph(lnf), _any_module(module_index == -1), _module_slot(module_slot), _module_index(module_index), _state(state)
{ 
  assert((module_index == -1 && module_slot == -1) || (module_index >= 0 && module_slot >= 0));
  startTimerHz(fps);
  state->add_any_listener(this);
}

void
module_graph::paint(Graphics& g)
{
  render_if_dirty();
  graph::paint(g);
}

void
module_graph::timerCallback()
{
  if(_done || !_render_dirty) return;
  render_if_dirty();
  repaint();
}

void
module_graph::render_if_dirty()
{
  if (!_render_dirty) return;
  int module_slot = _module_slot;
  int module_index = _module_index;
  if (_any_module)
  {
    if(_tweaked_param == -1) return;
    auto const& mapping = _state->desc().param_mappings.params[_tweaked_param];
    module_slot = mapping.topo.module_slot;
    module_index = mapping.topo.module_index;
  }

  auto const& module = _state->desc().plugin->modules[module_index];
  if(module.graph_renderer != nullptr)
    render(module.graph_renderer(*_state, module_slot));
  _render_dirty = false;
}

void 
module_graph::any_state_changed(int param, plain_value plain)
{
  auto const& mapping = _state->desc().param_mappings.params[param];
  if (_state->desc().plugin->modules[mapping.topo.module_index].params[mapping.topo.param_index].dsp.direction == param_direction::output) return;
  if(!_any_module && (mapping.topo.module_index != _module_index || mapping.topo.module_slot != _module_slot)) return;
  _tweaked_param = param;
  _render_dirty = true;
}

void 
graph::render(graph_data const& data)
{
  _data = data.data;
  if(data.bipolar)
    for (int i = 0; i < _data.size(); i++)
      _data[i] = bipolar_to_unipolar(_data[i]);
  for (int i = 0; i < _data.size(); i++)
    _data[i] = 1 - _data[i];
  repaint();
}

void
graph::paint(Graphics& g)
{
  Path p;
  float vpad = 1;
  float w = getWidth();
  float h = getHeight();
  g.fillAll(_lnf->colors().graph_background);

  int grid_rows = 5;
  int grid_cols = 13;
  g.setColour(_lnf->colors().graph_grid.withAlpha(0.25f));
  for(int i = 1; i <= grid_rows; i++)
    g.fillRect(0.0f, i / (float)(grid_rows + 1) * h, w, 1.0f);
  for (int i = 1; i <= grid_cols; i++)
    g.fillRect(i / (float)(grid_cols + 1) * w, 0.0f, 1.0f, h);

  if (_data.size() == 0) return;
  float count = _data.size();
  p.startNewSubPath(0, vpad + _data[0] * (h - 2 * vpad));
  for(int i = 1; i < _data.size(); i++)
    p.lineTo(i / count * w, vpad + _data[i] * (h - 2 * vpad));
  Path pStroke(p);
  p.closeSubPath();
  
  g.setColour(_lnf->colors().graph_foreground.withAlpha(0.5f));
  g.fillPath(p);
  g.setColour(_lnf->colors().graph_foreground);
  g.strokePath(pStroke, PathStrokeType(1));
}

}