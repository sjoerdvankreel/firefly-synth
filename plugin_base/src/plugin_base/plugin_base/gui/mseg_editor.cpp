#include <plugin_base/gui/mseg_editor.hpp>

using namespace juce;

namespace plugin_base {

mseg_editor::
mseg_editor(
  plugin_gui* gui, lnf* lnf, int module_index, int module_slot, 
  int start_y_param, int end_y_param, int on_param, int x_param, int y_param, int slope_param):
_gui(gui), _lnf(lnf),
_module_index(module_index), _module_slot(module_slot), 
_start_y_param(start_y_param), _end_y_param(end_y_param),
_on_param(on_param), _x_param(x_param), _y_param(y_param), _slope_param(slope_param)
{
  assert(gui != nullptr);
  assert(lnf != nullptr);
  auto const& topo = *gui->automation_state()->desc().plugin;
  auto const& param_list = topo.modules[module_index].params;
  (void)param_list;

  auto is_linear_unit = [](param_topo const& pt) { 
    return pt.domain.type == domain_type::linear && pt.domain.min == 0 && pt.domain.max == 1; };
  assert(is_linear_unit(param_list[start_y_param]));
  assert(is_linear_unit(param_list[end_y_param]));
  assert(is_linear_unit(param_list[x_param]));
  assert(is_linear_unit(param_list[y_param]));
  assert(is_linear_unit(param_list[slope_param]));
  assert(param_list[start_y_param].info.slot_count == 1);
  assert(param_list[end_y_param].info.slot_count == 1);
  assert(param_list[x_param].info.slot_count >= 1);
  assert(param_list[y_param].info.slot_count == param_list[x_param].info.slot_count);
  assert(param_list[on_param].info.slot_count == param_list[x_param].info.slot_count);
  assert(param_list[slope_param].info.slot_count == param_list[x_param].info.slot_count + 1);
}

void
mseg_editor::paint(Graphics& g)
{
  float const point_size = 8.0f;
  float const line_thickness = 2.0f;
  float const padding = point_size * 0.5f + 2;

  float const x = padding;
  float const y = padding;
  float const w = getLocalBounds().getWidth() - padding * 2.0f;
  float const h = getLocalBounds().getHeight() - padding * 2.0f;

  auto const state = _gui->automation_state();
  float end_y = state->get_plain_at(_module_index, _module_slot, _end_y_param, 0).real();
  float start_y = state->get_plain_at(_module_index, _module_slot, _start_y_param, 0).real();

  std::vector<std::pair<float, float>> points = {};
  int max_point_count = state->desc().plugin->modules[_module_index].params[_on_param].info.slot_count;
  for(int i = 0; i < max_point_count; i++)
    if (state->get_plain_at(_module_index, _module_slot, _on_param, i).step() != 0)
    {
      float x_norm = state->get_plain_at(_module_index, _module_slot, _x_param, i).real();
      float y_norm = state->get_plain_at(_module_index, _module_slot, _y_param, i).real();
      points.push_back({ x_norm, y_norm });
    }

  // dsp also needs to sort!
  std::sort(points.begin(), points.end(), [](auto const& l, auto const& r) { return l.first < r.first; });

  g.setColour(_lnf->colors().mseg_background);
  g.fillRect(getLocalBounds());

  // start to point 0
  g.setColour(_lnf->colors().mseg_line);
  g.drawLine(x, y + h - start_y * h, x + w * points[0].first, y + h - h * points[0].second);
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size, 1);
  g.drawEllipse(x + w * points[0].first - point_size / 2, y + h - h * points[0].second - point_size / 2, point_size, point_size, 1);

  // mid sections
  for (int i = 1; i < points.size(); i++)
  {
    Path p;
    float x1_norm = points[i - 1].first;
    float x2_norm = points[i].first;
    float y1_norm = points[i - 1].second;
    float y2_norm = points[i].second;

    int const pixel_count = (int)std::ceil((x2_norm - x1_norm) * w);
    auto sloped_y = [this, i, &state](float pos, float y1, float y2) {
      float slope = state->get_plain_at(_module_index, _module_slot, _slope_param, i - 1).real();
      if (slope < 0.5f) slope = 0.1f + 0.9f * slope * 2.0f;
      else slope = 1.0f + 9.0f * (slope - 0.5f) * 2.0f;
      return std::pow(pos, slope) * (y2 - y1);
    };
    p.startNewSubPath(x + w * x1_norm, y + h - h * y1_norm);
    for (int j = 1; j < pixel_count; j++)
    {
      float pos = j / (pixel_count - 1.0f);
      float x_this_pos_norm = x1_norm + pos * (x2_norm - x1_norm);
      float y_this_pos_norm = y1_norm + sloped_y(pos, y1_norm, y2_norm); // todo get the curvature in here
      float x_this_pos = x + w * x_this_pos_norm;
      float y_this_pos = y + h - h * y_this_pos_norm;
      p.lineTo(x_this_pos, y_this_pos);
    }    

    g.setColour(_lnf->colors().mseg_line);
    g.strokePath(p, PathStrokeType(line_thickness)); 
    g.setColour(_lnf->colors().mseg_point);
    g.drawEllipse(x + w * points[i].first - point_size / 2, y + h - h * points[i].second - point_size / 2, point_size, point_size, 1);
  }

  // last to end point
  g.setColour(_lnf->colors().mseg_line);
  g.drawLine(x + w * points[points.size() - 1].first, y + h - h * points[points.size() - 1].second, x + w - 1, y + h - end_y * h);
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size, 1);
}

}