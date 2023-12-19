#include <plugin_base/gui/graph.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

using namespace juce;

namespace plugin_base {

module_graph::
~module_graph() 
{ 
  _done = true;
  stopTimer();
  if(_params.render_on_tweak)
    _gui->gui_state()->remove_any_listener(this);
  if(_params.render_on_hover)
    _gui->remove_gui_mouse_listener(this);
}

module_graph::
module_graph(plugin_gui* gui, lnf* lnf, module_graph_params const& params):
graph(lnf), _gui(gui), _params(params)
{ 
  assert(params.fps > 0);
  assert(params.render_on_tweak || params.render_on_hover);
  if(_params.render_on_hover)
    gui->add_gui_mouse_listener(this);
  if(_params.render_on_tweak)
    gui->gui_state()->add_any_listener(this);
  startTimerHz(params.fps);
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
module_graph::any_state_changed(int param, plain_value plain) 
{
  auto const& desc = _gui->gui_state()->desc();
  auto const& mapping = desc.param_mappings.params[param];
  if(_params.module == -1 || _params.module == mapping.topo.module_index)
    request_rerender(param);
}

void 
module_graph::module_mouse_exit(int module) 
{ 
  auto const& desc = _gui->gui_state()->desc().modules[module];
  if (_params.module != -1 && _params.module != desc.module->info.index) return;
  setTooltip(String());
  render(graph_data()); 
}

void
module_graph::module_mouse_enter(int module)
{
  // trigger re-render based on first new module param
  auto const& desc = _gui->gui_state()->desc().modules[module];
  if (_params.module != -1 && _params.module != desc.module->info.index) return;
  if(desc.params.size() == 0) return;
  if (!_gui->gui_state()->desc().modules[module].module->rerender_on_param_hover)
    request_rerender(desc.params[0].info.global);
}

void
module_graph::param_mouse_enter(int param)
{
  // trigger re-render based on specific param
  auto const& mapping = _gui->gui_state()->desc().param_mappings.params[param];
  if (_params.module != -1 && _params.module != mapping.topo.module_index) return;
  if (_gui->gui_state()->desc().plugin->modules[mapping.topo.module_index].rerender_on_param_hover)
    request_rerender(param);
}

void
module_graph::request_rerender(int param)
{
  auto const& desc = _gui->gui_state()->desc();
  auto const& mapping = desc.param_mappings.params[param];
  int m = mapping.topo.module_index;
  int p = mapping.topo.param_index;
  if (desc.plugin->modules[m].params[p].dsp.direction == param_direction::output) return;
  if (desc.plugin->modules[mapping.topo.module_index].rerender_on_param_hover)
    setTooltip(desc.params[param]->full_name);
  else
    setTooltip(desc.modules[mapping.module_global].info.name);
  _render_dirty = true;
  _hovered_or_tweaked_param = param;
}

void
module_graph::render_if_dirty()
{
  if (!_render_dirty) return;
  if (_hovered_or_tweaked_param == -1) return;

  auto const& mappings = _gui->gui_state()->desc().param_mappings.params;
  param_topo_mapping mapping = mapping = mappings[_hovered_or_tweaked_param].topo;
  auto const& module = _gui->gui_state()->desc().plugin->modules[mapping.module_index];
  if(module.graph_renderer != nullptr)
    render(module.graph_renderer(*_gui->gui_state(), mapping));
  _render_dirty = false;
}

void 
graph::render(graph_data const& data)
{
  _data = data;
  repaint();
}

void 
graph::paint_series(Graphics& g, jarray<float, 1> const& series)
{
  Path p;
  float w = getWidth();
  float h = getHeight();
  float count = series.size();

  auto foreground = _lnf->colors().graph_foreground;
  if(_data.type() == graph_data_type::empty)
    foreground = color_to_grayscale(foreground);

  p.startNewSubPath(0, (1 - std::clamp(series[0], 0.0f, 1.0f)) * h);
  for (int i = 1; i < series.size(); i++)
    p.lineTo(i / count * w, (1 - std::clamp(series[i], 0.0f, 1.0f)) * h);
  Path pStroke(p);
  p.closeSubPath();

  g.setColour(foreground.withAlpha(0.5f));
  g.fillPath(p);
  g.setColour(foreground);
  g.strokePath(pStroke, PathStrokeType(1));
}

void
graph::paint(Graphics& g)
{
  Path p;
  float w = getWidth();
  float h = getHeight();
  g.fillAll(_lnf->colors().graph_background);

  int grid_rows = 5;
  int grid_cols = 13;
  if (_data.type() == graph_data_type::audio)
  {
    grid_rows = 7;
    grid_cols = 17;
  }

  g.setColour(_lnf->colors().graph_grid.withAlpha(0.25f));
  for(int i = 1; i <= grid_rows; i++)
    g.fillRect(0.0f, i / (float)(grid_rows + 1) * h, w, 1.0f);
  for (int i = 1; i <= grid_cols; i++)
    g.fillRect(i / (float)(grid_cols + 1) * w, 0.0f, 1.0f, h);

  auto foreground = _lnf->colors().graph_foreground;
  if (_data.type() == graph_data_type::scalar)
  {
    float scalar = _data.scalar();
    if (_data.bipolar())
    {
      scalar = 1.0f - bipolar_to_unipolar(scalar);
      g.setColour(foreground.withAlpha(0.5f));
      if (scalar <= 0.5f)
        g.fillRect(0.0f, scalar * h, w, (0.5f - scalar) * h);
      else
        g.fillRect(0.0f, 0.5f * h, w, (scalar - 0.5f) * h);
      g.setColour(foreground);
      g.fillRect(0.0f, scalar * h, w, 1.0f);
    }
    else
    {
      scalar = 1.0f - scalar;
      g.setColour(foreground.withAlpha(0.5f));
      g.fillRect(0.0f, scalar * h, w, (1 - scalar) * h);
      g.setColour(foreground);
      g.fillRect(0.0f, scalar * h, w, 1.0f);
    }
    return;
  }

  if (_data.type() == graph_data_type::audio)
  {
    jarray<float, 2> audio(_data.audio());
    for(int c = 0; c < 2; c++)
      for (int i = 0; i < audio[c].size(); i++)
        audio[c][i] = ((1 - c) + std::clamp(bipolar_to_unipolar(audio[c][i]), 0.0f, 1.0f)) * 0.5f;
    paint_series(g, audio[0]);
    paint_series(g, audio[1]);
    return;
  }

  graph_data data;
  if(_data.type() == graph_data_type::series)
    data = _data;
  else
  {
    assert(_data.type() == graph_data_type::empty);
    jarray<float, 1> empty;
    for (int i = 0; i < 33; i++)
      empty.push_back(0);
    for (int i = 0; i < 34; i++)
      empty.push_back(std::sin((float)i / 33 * 2.0f * pi32));
    for (int i = 0; i < 33; i++)
      empty.push_back(0);
    data = graph_data(empty, true);
  }

  jarray<float, 1> series(data.series());
  if(data.bipolar())
    for(int i = 0; i < series.size(); i++)
      series[i] = bipolar_to_unipolar(series[i]);
  paint_series(g, series);
}

}