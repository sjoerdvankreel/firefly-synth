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

float 
mseg_editor::sloped_y_pos(
  float pos, int index, 
  float y1, float y2) const
{
  auto const state = _gui->automation_state();
  float slope = state->get_plain_at(_module_index, _module_slot, _slope_param, index).real();
  if (slope < 0.5f) slope = 0.01f + 0.99f * slope * 2.0f;
  else slope = 1.0f + 99.0f * (slope - 0.5f) * 2.0f;
  return y1 + std::pow(pos, slope) * (y2 - y1); 
}

void 
mseg_editor::make_slope_path(
  float x, float y, float w, float h, 
  std::pair<float, float> const& from, 
  std::pair<float, float> const& to, 
  int slope_index, bool closed, Path& path) const
{
  path = {};
  float x1_norm = from.first;
  float x2_norm = to.first;
  float y1_norm = from.second;
  float y2_norm = to.second;

  int const pixel_count = (int)std::ceil((x2_norm - x1_norm) * w);
  if (closed)
  {
    path.startNewSubPath(x + w * x1_norm, y + h);
    path.lineTo(x + w * x1_norm, y + h - h * y1_norm);
  }
  else
    path.startNewSubPath(x + w * x1_norm, y + h - h * y1_norm);
  for (int j = 1; j < pixel_count; j++)
  {
    float pos = j / (pixel_count - 1.0f);
    float x_this_pos_norm = x1_norm + pos * (x2_norm - x1_norm);
    float y_this_pos_norm = sloped_y_pos(pos, slope_index, y1_norm, y2_norm);
    float x_this_pos = x + w * x_this_pos_norm;
    float y_this_pos = y + h - h * y_this_pos_norm;
    path.lineTo(x_this_pos, y_this_pos);

    if (closed && j == pixel_count - 1)
    {
      path.lineTo(x_this_pos, y + h);
      path.closeSubPath();
    }
  }
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

  Path sloped_path;
  float slope_marker_x;
  float slope_marker_y;
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

  g.setColour(_lnf->colors().mseg_grid);
  g.drawRect(getLocalBounds(), 1.0f);

  // start to point 0
  g.setColour(_lnf->colors().mseg_line);
  make_slope_path(x, y, w, h, { 0.0f, start_y }, points[0], 0, false, sloped_path);
  g.strokePath(sloped_path, PathStrokeType(line_thickness));
  g.setColour(_lnf->colors().mseg_area);
  make_slope_path(x, y, w, h, { 0.0f, start_y }, points[0], 0, true, sloped_path);
  g.fillPath(sloped_path);

  // point marker
  g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size);
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size, 1);

  // point marker
  g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x + w * points[0].first - point_size / 2, y + h - h * points[0].second - point_size / 2, point_size, point_size);
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(x + w * points[0].first - point_size / 2, y + h - h * points[0].second - point_size / 2, point_size, point_size, 1);

  // slope marker
  slope_marker_x = x + (0.0f + points[0].first) * 0.5f * w - point_size / 2;
  slope_marker_y = y + h - h * sloped_y_pos(0.5f, 0, start_y, points[0].second) - point_size / 2;
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);

  // mid sections
  for (int i = 1; i < points.size(); i++)
  {
    // point n - 1 to n
    g.setColour(_lnf->colors().mseg_line);
    make_slope_path(x, y, w, h, points[i - 1], points[i], i, false, sloped_path);
    g.strokePath(sloped_path, PathStrokeType(line_thickness));
    g.setColour(_lnf->colors().mseg_area);
    make_slope_path(x, y, w, h, points[i - 1], points[i], i, true, sloped_path);
    g.fillPath(sloped_path);

    // point marker
    g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
    g.fillEllipse(x + w * points[i].first - point_size / 2, y + h - h * points[i].second - point_size / 2, point_size, point_size);
    g.setColour(_lnf->colors().mseg_point);
    g.drawEllipse(x + w * points[i].first - point_size / 2, y + h - h * points[i].second - point_size / 2, point_size, point_size, 1);

    // slope marker
    slope_marker_x = x + (points[i - 1].first + points[i].first) * 0.5f * w - point_size / 2;
    slope_marker_y = y + h - h * sloped_y_pos(0.5f, i, points[i - 1].second, points[i].second) - point_size / 2;
    g.setColour(_lnf->colors().mseg_point);
    g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);
  }

  // last to end point
  g.setColour(_lnf->colors().mseg_line);
  make_slope_path(x, y, w, h, points[points.size() - 1], { 1.0f, end_y }, points.size(), false, sloped_path);
  g.strokePath(sloped_path, PathStrokeType(line_thickness));
  g.setColour(_lnf->colors().mseg_area);
  make_slope_path(x, y, w, h, points[points.size() - 1], { 1.0f, end_y }, points.size(), true, sloped_path);
  g.fillPath(sloped_path);

  // point marker
  g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size);
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size, 1);

  // slope marker
  slope_marker_x = x + (points[points.size() - 1].first + 1.0f) * 0.5f * w - point_size / 2;
  slope_marker_y = y + h - h * sloped_y_pos(0.5f, points.size(), points[points.size() - 1].second, end_y) - point_size / 2;
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);
}

}