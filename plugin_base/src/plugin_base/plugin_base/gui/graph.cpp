#include <plugin_base/gui/graph.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>
#include <plugin_base/shared/utility.hpp>

using namespace juce;

namespace plugin_base {

module_graph::
~module_graph() 
{ 
  _done = true;
  stopTimer();
  _gui->remove_modulation_output_listener(this);
  if(_module_params.render_on_tweak) _gui->automation_state()->remove_any_listener(this);
  if(_module_params.render_on_tab_change) _gui->remove_tab_selection_listener(this);
  if (_module_params.render_on_module_mouse_enter || _module_params.render_on_param_mouse_enter_modules.size())
    _gui->remove_gui_mouse_listener(this);
}

module_graph::
module_graph(plugin_gui* gui, lnf* lnf, graph_params const& params, module_graph_params const& module_params):
graph(gui, lnf, params), _module_params(module_params)
{ 
  assert(_module_params.fps > 0);
  assert(_module_params.render_on_tweak || _module_params.render_on_tab_change ||
    _module_params.render_on_module_mouse_enter || _module_params.render_on_param_mouse_enter_modules.size());
  if(_module_params.render_on_tab_change) assert(_module_params.module_index != -1);
  if (_module_params.render_on_tweak) gui->automation_state()->add_any_listener(this);
  if(_module_params.render_on_module_mouse_enter || _module_params.render_on_param_mouse_enter_modules.size())
    gui->add_gui_mouse_listener(this);
  if (_module_params.render_on_tab_change)
  {
    gui->add_tab_selection_listener(this);
    module_tab_changed(_module_params.module_index, 0);
  }
  _gui->add_modulation_output_listener(this);
  startTimerHz(_module_params.fps);
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
  if(render_if_dirty())
    repaint();
}

void
module_graph::modulation_outputs_reset()
{
  if (_hovered_or_tweaked_param == -1)
    return;

  _mod_indicators.clear();
  _custom_outputs.clear();
  _render_dirty = true;
  request_rerender(_hovered_or_tweaked_param, false);
}

void 
module_graph::modulation_outputs_changed(std::vector<modulation_output> const& outputs)
{
  if (_hovered_or_tweaked_param == -1)
    return;

  // we still get the events, so need to back off.
  // it is this way because the param sliders still need the events for param-only mode
  if (_gui->get_visuals_mode() != gui_visuals_mode_full)
    return;

  int this_module_slot = -1;
  int this_module_index = -1;
  auto const& desc = _gui->automation_state()->desc();
  auto const& mappings = desc.param_mappings.params;
  param_topo_mapping mapping = mappings[_hovered_or_tweaked_param].topo;

  if (_module_params.module_index != -1)
  {
    this_module_slot = _activated_module_slot;
    this_module_index = _module_params.module_index;
  }
  else
  {
    this_module_slot = desc.param_mappings.params[_hovered_or_tweaked_param].topo.module_slot;
    this_module_index = desc.param_mappings.params[_hovered_or_tweaked_param].topo.module_index;
  }

  int this_module_global = desc.module_topo_to_index.at(this_module_index) + this_module_slot;
  int this_param_first = desc.modules[this_module_global].params[0].info.global;

  if (!desc.modules[this_module_global].module->gui.rerender_graph_on_modulation)
    return;

  bool rerender_indicators = false;
  bool any_mod_indicator_found = false;
  for (int i = 0; i < outputs.size(); i++)
    if (outputs[i].event_type() == output_event_type::out_event_cv_state)
      if (this_module_global == outputs[i].state.cv.module_global)
      {
        if (!any_mod_indicator_found)
        {
          any_mod_indicator_found = true;
          _mod_indicators.clear();
          _mod_indicators_activated = seconds_since_epoch();
        }
        _mod_indicators.push_back(outputs[i].state.cv.position_normalized);
      }

  if (!any_mod_indicator_found && seconds_since_epoch() >= _mod_indicators_activated + 1.0)
  {
    _mod_indicators.clear();
    rerender_indicators = true;
  }

  bool rerender_full = false;
  bool any_custom_found = false;
  auto const& dependent_custom_outputs_module_topo_indices = desc.modules[this_module_global].module->dependent_custom_outputs_module_topo_indices;
  for (int i = 0; i < outputs.size(); i++)
    if (outputs[i].event_type() == output_event_type::out_event_custom_state)
    {
      int event_module_topo_index = desc.modules[outputs[i].state.custom.module_global].module->info.index;
      if (this_module_global == outputs[i].state.custom.module_global ||
        (std::find(dependent_custom_outputs_module_topo_indices.begin(),
          dependent_custom_outputs_module_topo_indices.end(),
          event_module_topo_index) != dependent_custom_outputs_module_topo_indices.end()))
      {
        if (!any_custom_found)
        {
          rerender_full = true;
          any_custom_found = true;
          _custom_outputs.clear();
          _custom_outputs_activated = seconds_since_epoch();
        }

        // note this builds up a list of stuff for us
        // plus stuff for anyone we want to repaint on (eg lfo stuff happens, we want to repaint cv matrix)
        // client code should take into account that stuff might not be their own module
        _custom_outputs.push_back(outputs[i].state.custom);
      }
    }

  if (!any_custom_found && seconds_since_epoch() >= _custom_outputs_activated + 1.0)
  {
    _custom_outputs.clear();
    rerender_full = true; // because we just dont know what else to do
  }

  if(!rerender_full)
    for (int i = 0; i < outputs.size(); i++)
      if (outputs[i].event_type() == output_event_type::out_event_param_state)
      {
        if (this_module_global == outputs[i].state.param.module_global)
        {
          rerender_full = true;
          break;
        }
      } else if (outputs[i].event_type() == output_event_type::out_event_cv_state)
      {
        if (this_module_global == outputs[i].state.cv.module_global)
        {
          rerender_indicators = true;
          // DONT break -- full rerender trumps indicators
        }
      }

  if (rerender_full)
    request_rerender(this_param_first, false);
  else if(rerender_indicators)
    repaint();
}

void 
module_graph::module_tab_changed(int module, int slot)
{
  // trigger re-render based on first new module param
  auto const& desc = _gui->automation_state()->desc();
  if(_module_params.module_index != -1 && _module_params.module_index != module) return;
  _activated_module_slot = slot;
  int index = desc.module_topo_to_index.at(module) + slot;
  _last_rerender_cause_param = desc.modules[index].params[0].info.global;
  request_rerender(_last_rerender_cause_param, false);
}

void 
module_graph::any_state_changed(int param, plain_value plain) 
{
  auto const& desc = _gui->automation_state()->desc();
  auto const& mapping = desc.param_mappings.params[param];
  if(_module_params.module_index == -1 || _module_params.module_index == mapping.topo.module_index)
  {
    if (_activated_module_slot == mapping.topo.module_slot)
    {
      if(_module_params.module_index != -1)
        _last_rerender_cause_param = param;
      request_rerender(param, false);
    }
    return;
  }

  // someone else changed a param and we depend on it
  // request re-render for first param in own topo or last changed in own topo
  if(std::find(
    _module_params.dependent_module_indices.begin(),
    _module_params.dependent_module_indices.end(),
    mapping.topo.module_index) == _module_params.dependent_module_indices.end())
    return;

  if(_last_rerender_cause_param != -1)
  {
    request_rerender(_last_rerender_cause_param, false);
    return;
  }  
  int index = desc.module_topo_to_index.at(_module_params.module_index) + _activated_module_slot;
  request_rerender(desc.modules[index].params[0].info.global, false);
}

void
module_graph::module_mouse_enter(int module)
{
  // trigger re-render based on first new module param
  auto const& desc = _gui->automation_state()->desc().modules[module];
  if (_module_params.module_index != -1 && _module_params.module_index != desc.module->info.index) return;
  if(desc.params.size() == 0) return;
  if(_module_params.render_on_module_mouse_enter && !desc.module->gui.force_rerender_graph_on_param_hover)
    request_rerender(desc.params[0].info.global, false);
}

void
module_graph::param_mouse_enter(int param)
{
  // trigger re-render based on specific param
  auto const& mapping = _gui->automation_state()->desc().param_mappings.params[param];
  if (_module_params.module_index != -1 && _module_params.module_index != mapping.topo.module_index) return;
  auto end = _module_params.render_on_param_mouse_enter_modules.end();
  auto begin = _module_params.render_on_param_mouse_enter_modules.begin();
  if (std::find(begin, end, mapping.topo.module_index) != end ||
    std::find(begin, end, -1) != end)
    request_rerender(param, true);
}

void
module_graph::request_rerender(int param, bool hover)
{
  auto const& desc = _gui->automation_state()->desc();
  auto const& mapping = desc.param_mappings.params[param];
  int m = mapping.topo.module_index;
  int p = mapping.topo.param_index;
  if (desc.plugin->modules[m].params[p].dsp.direction == param_direction::output) return;
  
  _render_dirty = true;
  _hovered_or_tweaked_param = param;
  if (hover) _hovered_param = param;
}

bool
module_graph::render_if_dirty()
{
  if (!_render_dirty) return false;
  if (_hovered_or_tweaked_param == -1) return false;

  int render_request_param = _hovered_or_tweaked_param;
  if(_module_params.hover_selects_different_graph && _hovered_param != -1)
    render_request_param = _hovered_param;
  if (render_request_param == -1) return false;

  auto const& mappings = _gui->automation_state()->desc().param_mappings.params;
  param_topo_mapping mapping = mappings[render_request_param].topo;
  auto const& module = _gui->automation_state()->desc().plugin->modules[mapping.module_index];

  gui_visuals_mode visuals_mode = _gui->get_visuals_mode();
  plugin_state const* plug_state = _gui->automation_state();
  int module_global = plug_state->desc().param_mappings.params[render_request_param].module_global;
  bool render_automation_state = plug_state->desc().modules[module_global].module->gui.render_automation_state;
  if (visuals_mode == gui_visuals_mode_full && !render_automation_state)
  {
    // find the latest active voice, otherwise go with global
    int voice_index = -1;
    std::uint32_t latest_timestamp = 0;
    if (module.dsp.stage == module_stage::voice)
      for (int i = 0; i < _gui->engine_voices_active().size(); i++)
        if (_gui->engine_voices_active()[i] != 0)
          if (_gui->engine_voices_activated()[i] > latest_timestamp)
            voice_index = i;
    plug_state = voice_index == -1 ? &_gui->global_modulation_state() : &_gui->voice_modulation_state(voice_index);
  }

  if(module.graph_renderer != nullptr)
    render(module.graph_renderer(
      *plug_state, _gui->get_module_graph_engine(module), render_request_param, mapping, _custom_outputs));
  _render_dirty = false;
  return true;
}

graph::
graph(plugin_gui* gui, lnf* lnf, graph_params const& params) :
_lnf(lnf), _data(graph_data_type::na, {}), _params(params), _gui(gui) {}

void 
graph::render(graph_data const& data)
{
  _data = data;
  repaint();
}

void 
graph::paint_series(
  Graphics& g, jarray<float, 1> const& series,
  bool bipolar, float stroke_thickness, float midpoint)
{
  Path pFill;
  Path pStroke;
  float w = getWidth();
  float h = getHeight();
  float count = series.size();

  float y0 = (1 - std::clamp(series[0], 0.0f, 1.0f)) * h;
  pFill.startNewSubPath(0, bipolar? h * midpoint : h);
  pFill.lineTo(0, y0);
  pStroke.startNewSubPath(0, y0);
  for (int i = 1; i < series.size(); i++)
  {
    float yn = (1 - std::clamp(series[i], 0.0f, 1.0f)) * h;
    pFill.lineTo(i / count * w, yn);
    pStroke.lineTo(i / count * w, yn);
  }
  pFill.lineTo(w, bipolar? h * midpoint : h);
  pFill.closeSubPath();

  g.setColour(_lnf->colors().graph_area);
  g.fillPath(pFill);
  if (!_data.stroke_with_area())
    g.setColour(_lnf->colors().graph_line);
  g.strokePath(pStroke, PathStrokeType(stroke_thickness));
}

void 
graph::paint_multi_bars(
  juce::Graphics& g, int implied_count,
  std::vector<std::pair<int, float>> const& multi_bars)
{
  float w = getWidth();
  float h = getHeight();

  for(int i = 0; i < implied_count ; i++)
    for (int j = 0; j < multi_bars.size(); j++)
      if(multi_bars[j].first == i)
      {
        g.setColour(_lnf->colors().graph_area);
        float bar_h = multi_bars[j].second * h;
        float bar_y = (1.0f - multi_bars[j].second) * h;
        float bar_w = w / implied_count;
        float bar_x = i / (float)implied_count * w;
        g.fillRect(bar_x, bar_y, bar_w, bar_h);

        g.setColour(_lnf->colors().graph_line);
        g.fillRect(bar_x, bar_y - 1.0f, bar_w, 2.0f);

        // for ARP
        // lets us see where note begins in case same chord is repeated
        g.fillEllipse(bar_x - 2, bar_y - 2, 4, 4);
    }
}

void
graph::paint(Graphics& g)
{
  Path p;
  float w = getWidth();
  float h = getHeight();
  g.fillAll(_lnf->colors().graph_background);

  // figure out grid box size such that row count is even and line 
  // count is uneven because we want a horizontal line in the middle
  float preferred_box_size = 9;
  int row_count = std::round(h / preferred_box_size);
  if(row_count % 2 != 0) row_count++;
  float box_size = h / row_count;
  int col_count = std::round(w / box_size);
  g.setColour(_lnf->colors().graph_grid);
  for(int i = 1; i < row_count; i++)
    g.fillRect(0.0f, i / (float)(row_count) * h, w, 1.0f);
  for (int i = 1; i < col_count; i++)
    g.fillRect(i / (float)(col_count) * w, 0.0f, 1.0f, h);

  // draw background partitions
  if(_data.type() != graph_data_type::off && _data.type() != graph_data_type::na)
    for (int part = 0; part < _data.partitions().size(); part++)
    {
      Rectangle<float> area(part / (float)_data.partitions().size() * w, 0.0f, w / _data.partitions().size(), h);
      if (part % 2 == 1)
      {
        g.setColour(_lnf->colors().graph_area.withAlpha(0.33f));
        g.fillRect(area);
      }
      g.setColour(_lnf->colors().graph_text);
      if (_params.scale_type == graph_params::scale_h)
        g.setFont(_lnf->font().withHeight(h * _params.partition_scale));
      else
        g.setFont(_lnf->font().withHeight(w * _params.partition_scale));
      g.drawText(_data.partitions()[part], area, Justification::centred, false);
    }
       
  if (_data.type() == graph_data_type::off || _data.type() == graph_data_type::na)
  {
    g.setColour(_lnf->colors().graph_text); 
    if (_params.scale_type == graph_params::scale_h)
      g.setFont(_lnf->font().withHeight(h * _params.partition_scale));
    else
      g.setFont(_lnf->font().withHeight(w * _params.partition_scale));
    std::string text = _data.type() == graph_data_type::off ? "OFF" : "N/A";
    assert(_data.partitions().size() < 2);
    if (_data.partitions().size() == 1) text = _data.partitions()[0] + " " + text;
    g.drawText(text, getLocalBounds().toFloat(), Justification::centred, false);
    return;
  }

  if (_data.type() == graph_data_type::multi_stereo)
  {
    auto area = _lnf->colors().graph_area;
    for (int i = 0; i < _data.multi_stereo().size(); i++)
    {
      float l = 1 - _data.multi_stereo()[i].first;
      float r = 1 - _data.multi_stereo()[i].second;
      g.setColour(area.withAlpha(0.67f / _data.multi_stereo().size()));
      g.fillRect(0.0f, l * h, w * 0.5f, (1 - l) * h);
      g.fillRect(w * 0.5f, r * h, w * 0.5f, (1 - r) * h);
      g.setColour(area);
      g.fillRect(0.0f, l * h, w * 0.5f, 1.0f);
      g.fillRect(w * 0.5f, r * h, w * 0.5f, 1.0f);
    }
    return;
  }

  if (_data.type() == graph_data_type::scalar)
  {
    auto area = _lnf->colors().graph_area;
    float scalar = _data.scalar();
    if (_data.bipolar())
    {
      scalar = 1.0f - bipolar_to_unipolar(scalar);
      g.setColour(area.withAlpha(0.67f));
      if (scalar <= 0.5f)
        g.fillRect(0.0f, scalar * h, w, (0.5f - scalar) * h);
      else
        g.fillRect(0.0f, 0.5f * h, w, (scalar - 0.5f) * h);
      g.setColour(area);
      g.fillRect(0.0f, scalar * h, w, 1.0f);
    }
    else
    {
      scalar = 1.0f - scalar;
      g.setColour(area.withAlpha(0.67f));
      g.fillRect(0.0f, scalar * h, w, (1 - scalar) * h);
      g.setColour(area);
      g.fillRect(0.0f, scalar * h, w, 1.0f);
    }
    return;
  }

  if (_data.type() == graph_data_type::audio)
  {
    jarray<float, 2> audio(_data.audio());
    for(int c = 0; c < 2; c++)
      for (int i = 0; i < audio[c].size(); i++)
        audio[c][i] = ((1 - c) + bipolar_to_unipolar(std::clamp(audio[c][i], -1.0f, 1.0f))) * 0.5f;
    paint_series(g, audio[0], true, _data.stroke_thickness(), 0.25f);
    paint_series(g, audio[1], true, _data.stroke_thickness(), 0.75f);
    return;
  }  

  assert(_data.type() == graph_data_type::series || _data.type() == graph_data_type::multi_bars);

  // paint the series / bars
  int count = -1;
  if (_data.type() == graph_data_type::multi_bars)
  {
    int max_index = 0;
    for (int i = 0; i < _data.multi_bars().size(); i++)
      max_index = std::max(max_index, _data.multi_bars()[i].first);
    count = max_index + 1;
    paint_multi_bars(g, count, _data.multi_bars());
  }
  else
  {
    count = _data.series().size();
    jarray<float, 1> series(_data.series());
    if (_data.bipolar())
      for (int i = 0; i < series.size(); i++)
        series[i] = bipolar_to_unipolar(series[i]);
    paint_series(g, series, _data.bipolar(), _data.stroke_thickness(), 0.5f);
  }

  if (_gui->get_visuals_mode() != gui_visuals_mode_full) return;

  // paint the indicator bubbles
  g.setColour(_lnf->colors().graph_modulation_bubble);
  for (int i = 0; i < _mod_indicators.size(); i++)
  {
    float output_pos = _mod_indicators[i];
    float x = output_pos * w;
    int point = std::clamp((int)(output_pos * count), 0, count - 1);
    if (_data.type() == graph_data_type::series)
    {
      float y = (1 - std::clamp(_data.series()[point], 0.0f, 1.0f)) * h;
      g.fillEllipse(x - 3, y - 3, 6, 6);
    }
    else
    {
      for (int j = 0; j < _data.multi_bars().size(); j++)
        if(_data.multi_bars()[j].first == point)
        {
          float y = (1 - std::clamp(_data.multi_bars()[j].second, 0.0f, 1.0f)) * h;
          g.fillEllipse(x - 3, y - 3, 6, 6);
        }
    }
  }
  if (fixed_graph_mod_indicator_pos() >= 0)
  {
    float x = fixed_graph_mod_indicator_pos() * w;
    int point = std::clamp((int)(fixed_graph_mod_indicator_pos() * (count - 1)), 0, count - 1);
    float y = (1 - std::clamp(_data.series()[point], 0.0f, 1.0f)) * h;
    g.fillEllipse(x - 3, y - 3, 6, 6);
  }
}

}