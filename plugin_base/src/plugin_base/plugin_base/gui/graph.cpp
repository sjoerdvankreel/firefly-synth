#include <plugin_base/gui/graph.hpp>
#include <plugin_base/dsp/utility.hpp>

using namespace juce;

namespace plugin_base {

module_graph::
~module_graph() 
{ 
  _done = true;
  _gui->gui_state()->remove_any_listener(this);
  _gui->remove_gui_listener(this);
  stopTimer();
}

module_graph::
module_graph(plugin_gui* gui, lnf* lnf, int module_index, int module_slot, int fps):
graph(lnf), _gui(gui), _any_module(module_index == -1), _module_slot(module_slot), _module_index(module_index)
{ 
  assert((module_index == -1 && module_slot == -1) || (module_index >= 0 && module_slot >= 0));
  startTimerHz(fps);
  gui->add_gui_listener(this);
  gui->gui_state()->add_any_listener(this);
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
module_graph::mouse_exit()
{
  if (!_any_module) return;
  graph_data empty;
  empty.series = false;
  empty.bipolar = true;
  empty.scalar_data = 0;
  render(empty);
}

void
module_graph::module_mouse_enter(int module)
{
  if (!_any_module) return;

  // trigger re-render based on first new module param
  auto const& params = _gui->gui_state()->desc().modules[module].params;
  if(params.size() == 0) return;
  if (!_gui->gui_state()->desc().modules[module].module->rerender_on_param_hover)
    any_state_changed(params[0].info.global, {});
}

void
module_graph::param_mouse_enter(int param)
{
  // trigger re-render based on specific param
  auto const& mapping = _gui->gui_state()->desc().param_mappings.params[param];
  if (_gui->gui_state()->desc().plugin->modules[mapping.topo.module_index].rerender_on_param_hover)
    any_state_changed(param, {});
}

void
module_graph::render_if_dirty()
{
  if (!_render_dirty) return;

  param_topo_mapping mapping;
  mapping.param_slot = 0;
  mapping.param_index = 0;
  mapping.module_slot = _module_slot;
  mapping.module_index = _module_index;
  if (_any_module)
  {
    if(_tweaked_param == -1) return;
    mapping = _gui->gui_state()->desc().param_mappings.params[_tweaked_param].topo;
  }

  auto const& module = _gui->gui_state()->desc().plugin->modules[mapping.module_index];
  if(module.graph_renderer != nullptr)
    render(module.graph_renderer(*_gui->gui_state(), mapping));
  _render_dirty = false;
}

void 
module_graph::any_state_changed(int param, plain_value plain)
{
  auto const& mapping = _gui->gui_state()->desc().param_mappings.params[param];
  if (_gui->gui_state()->desc().plugin->modules[mapping.topo.module_index].params[mapping.topo.param_index].dsp.direction == param_direction::output) return;
  if(!_any_module && (mapping.topo.module_index != _module_index || mapping.topo.module_slot != _module_slot)) return;
  _tweaked_param = param;
  _render_dirty = true;
}

void 
graph::render(graph_data const& data)
{
  _data = data;
  if (!data.series)
  {
    if(data.bipolar)
      _data.scalar_data = bipolar_to_unipolar(data.scalar_data);
    _data.scalar_data = 1 - _data.scalar_data;
  }
  else
  {
    if(data.bipolar)
      for (int i = 0; i < _data.series_data.size(); i++)
        _data.series_data[i] = bipolar_to_unipolar(_data.series_data[i]);
    for (int i = 0; i < _data.series_data.size(); i++)
      _data.series_data[i] = 1 - _data.series_data[i];
  }
  repaint();
}

void
graph::paint(Graphics& g)
{
  Path p;
  float vpad = 1;
  float w = getWidth();
  float full_h = getHeight();
  float pad_h = full_h - 2 * vpad;
  g.fillAll(_lnf->colors().graph_background);

  int grid_rows = 5;
  int grid_cols = 13;
  g.setColour(_lnf->colors().graph_grid.withAlpha(0.25f));
  for(int i = 1; i <= grid_rows; i++)
    g.fillRect(0.0f, i / (float)(grid_rows + 1) * full_h, w, 1.0f);
  for (int i = 1; i <= grid_cols; i++)
    g.fillRect(i / (float)(grid_cols + 1) * w, 0.0f, 1.0f, full_h);

  if (!_data.series)
  {
    if (_data.bipolar)
    {
      g.setColour(_lnf->colors().graph_foreground.withAlpha(0.5f));
      if (_data.scalar_data <= 0.5f)
        g.fillRect(0.0f, _data.scalar_data * full_h, w, (0.5f - _data.scalar_data) * pad_h);
      else
        g.fillRect(0.0f, 0.5f * full_h, w, (_data.scalar_data - 0.5f) * pad_h);
      g.setColour(_lnf->colors().graph_foreground);
      g.fillRect(0.0f, _data.scalar_data * pad_h, w, 1.0f);
    }
    else 
    {
      g.setColour(_lnf->colors().graph_foreground.withAlpha(0.5f));
      g.fillRect(0.0f, _data.scalar_data * full_h, w, (1 - _data.scalar_data) * pad_h);
      g.setColour(_lnf->colors().graph_foreground);
      g.fillRect(0.0f, _data.scalar_data * pad_h, w, 1.0f);
    }
    return;
  }

  if (_data.series && _data.series_data.size() == 0)
  {
    g.setColour(_lnf->colors().graph_foreground);
    g.fillRect(0.0f, full_h / 2.0f, w, 1.0f);
    return;
  }

  float count = _data.series_data.size();
  p.startNewSubPath(0, vpad + _data.series_data[0] * pad_h);
  for(int i = 1; i < _data.series_data.size(); i++)
    p.lineTo(i / count * w, vpad + _data.series_data[i] * pad_h);
  Path pStroke(p);
  p.closeSubPath();
  
  g.setColour(_lnf->colors().graph_foreground.withAlpha(0.5f));
  g.fillPath(p);
  g.setColour(_lnf->colors().graph_foreground);
  g.strokePath(pStroke, PathStrokeType(1));
}

}