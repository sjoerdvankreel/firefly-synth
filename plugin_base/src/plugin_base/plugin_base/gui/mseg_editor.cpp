#include <plugin_base/gui/mseg_editor.hpp>

using namespace juce;

namespace plugin_base {

static float const point_size = 8.0f;
static float const line_thickness = 2.0f;
static float const padding = point_size * 0.5f + 2;
  
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
    return pt.domain.type == domain_type::identity && pt.domain.min == 0 && pt.domain.max == 1; };
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
  double const slope_min = exp_slope_min;
  double const slope_max = (1.0 - exp_slope_min);
  double const slope_range = slope_max - slope_min;
  double const slope_bounded = exp_slope_min + slope_range * slope;
  double const exp = std::log(slope_bounded) / std::log(0.5);
  return y1 + std::pow(pos, exp) * (y2 - y1); 
}

void
mseg_editor::calc_sorted_points()
{
  _sorted_points.clear();
  auto const state = _gui->automation_state();
  int max_point_count = state->desc().plugin->modules[_module_index].params[_on_param].info.slot_count;
  for (int i = 0; i < max_point_count; i++)
    if (state->get_plain_at(_module_index, _module_slot, _on_param, i).step() != 0)
    {
      float x_norm = state->get_plain_at(_module_index, _module_slot, _x_param, i).real();
      float y_norm = state->get_plain_at(_module_index, _module_slot, _y_param, i).real();
      _sorted_points.push_back({ x_norm, y_norm });
    }

  // dsp also must sort !
  std::sort(_sorted_points.begin(), _sorted_points.end(), [](auto const& l, auto const& r) { return l.first < r.first; });
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
mseg_editor::mouseUp(MouseEvent const& event)
{
  _dragging_point = -1;
  _dragging_slope = -1;
  _dragging_end_y = false;
  _dragging_start_y = false;
}

void
mseg_editor::mouseDown(MouseEvent const& event)
{
  _dragging_point = _hovered_point;
  _dragging_slope = _hovered_slope;
  _dragging_end_y = _hovered_end_y;
  _dragging_start_y = _hovered_start_y;
}

void
mseg_editor::mouseDrag(MouseEvent const& event)
{
  Image image(Image::PixelFormat::ARGB, point_size, point_size, true);
  Graphics g(image);
  if (_dragging_slope != -1)
  {
    g.setColour(_lnf->colors().mseg_line);
    g.drawEllipse(0.0f, 0.0f, point_size, point_size, 1.0f);
  }
  else
  {
    g.setColour(_lnf->colors().mseg_line);
    g.drawEllipse(0.0f, 0.0f, point_size, point_size, 1.0f);
    g.setColour(_lnf->colors().mseg_line.withAlpha(0.5f));
    g.fillEllipse(0.0f, 0.0f, point_size, point_size);
  }
  Point<int> offset(image.getWidth() / 2 + point_size, image.getHeight() / 2 + point_size);
  startDragging(String(""), this, ScaledImage(image), false, &offset);
}

void
mseg_editor::mouseMove(MouseEvent const& event)
{
  float const x = padding;
  float const y = padding;
  float const w = getLocalBounds().getWidth() - padding * 2.0f;
  float const h = getLocalBounds().getHeight() - padding * 2.0f;

  auto const state = _gui->automation_state();
  float end_y = state->get_plain_at(_module_index, _module_slot, _end_y_param, 0).real();
  float start_y = state->get_plain_at(_module_index, _module_slot, _start_y_param, 0).real();

  int prev_hovered_point = _hovered_point;
  int prev_hovered_slope = _hovered_slope;
  bool prev_hovered_end_y = _hovered_end_y;
  bool prev_hovered_start_y = _hovered_start_y;

  _hovered_point = -1;
  _hovered_slope = -1;
  _hovered_end_y = false;
  _hovered_start_y = false;
  setMouseCursor(MouseCursor::ParentCursor);
  calc_sorted_points();

  // todo this is bound to cause overlapping on near points ...

  float start_y_x1 = x - point_size / 2;
  float start_y_y1 = y + h - start_y * h - point_size / 2;
  float start_y_x2 = start_y_x1 + point_size;
  float start_y_y2 = start_y_y1 + point_size;
  if (start_y_x1 <= event.x && event.x <= start_y_x2 && start_y_y1 <= event.y && event.y <= start_y_y2)
  {
    _hovered_point = -1;
    _hovered_slope = -1;
    _hovered_end_y = false;
    _hovered_start_y = true;
    setMouseCursor(MouseCursor::DraggingHandCursor);
  }

  float start_slope_x1 = x + (0.0f + _sorted_points[0].first) * 0.5f * w - point_size / 2;
  float start_slope_y1 = y + h - h * sloped_y_pos(0.5f, 0, start_y, _sorted_points[0].second) - point_size / 2;
  float start_slope_x2 = start_slope_x1 + point_size;
  float start_slope_y2 = start_slope_y1 + point_size;
  if (start_slope_x1 <= event.x && event.x <= start_slope_x2 && start_slope_y1 <= event.y && event.y <= start_slope_y2)
  {
    _hovered_point = -1;
    _hovered_slope = 0;
    _hovered_end_y = false;
    _hovered_start_y = false;
    setMouseCursor(MouseCursor::DraggingHandCursor);
  }

  for (int i = 0; i < _sorted_points.size(); i++)
  {
    float this_point_x1 = x + w * _sorted_points[i].first - point_size / 2;
    float this_point_y1 = y + h - h * _sorted_points[i].second - point_size / 2;
    float this_point_x2 = this_point_x1 + point_size;
    float this_point_y2 = this_point_y1 + point_size;
    if (this_point_x1 <= event.x && event.x <= this_point_x2 && this_point_y1 <= event.y && event.y <= this_point_y2)
    {
      _hovered_point = i;
      _hovered_slope = -1;
      _hovered_end_y = false;
      _hovered_start_y = false;
      setMouseCursor(MouseCursor::DraggingHandCursor);
    }

    if (i > 0)
    {
      float this_slope_x1 = x + (_sorted_points[i - 1].first + _sorted_points[i].first) * 0.5f * w - point_size / 2;
      float this_slope_y1 = y + h - h * sloped_y_pos(0.5f, i, _sorted_points[i - 1].second, _sorted_points[i].second) - point_size / 2;
      float this_slope_x2 = this_slope_x1 + point_size;
      float this_slope_y2 = this_slope_y1 + point_size;
      if (this_slope_x1 <= event.x && event.x <= this_slope_x2 && this_slope_y1 <= event.y && event.y <= this_slope_y2)
      {
        _hovered_point = -1;
        _hovered_slope = i;
        _hovered_end_y = false;
        _hovered_start_y = false;
        setMouseCursor(MouseCursor::DraggingHandCursor);
      }
    }
  }

  float last_slope_x1 = x + (_sorted_points[_sorted_points.size() - 1].first + 1.0f) * 0.5f * w - point_size / 2;
  float last_slope_y1 = y + h - h * sloped_y_pos(0.5f, _sorted_points.size(), _sorted_points[_sorted_points.size() - 1].second, end_y) - point_size / 2;
  float last_slope_x2 = last_slope_x1 + point_size;
  float last_slope_y2 = last_slope_y1 + point_size;
  if (last_slope_x1 <= event.x && event.x <= last_slope_x2 && last_slope_y1 <= event.y && event.y <= last_slope_y2)
  {
    _hovered_point = -1;
    _hovered_slope = _sorted_points.size();
    _hovered_end_y = false;
    _hovered_start_y = false;
    setMouseCursor(MouseCursor::DraggingHandCursor);
  }

  float end_y_x1 = x + w - 1 - point_size / 2;
  float end_y_y1 = y + h - end_y * h - point_size / 2;
  float end_y_x2 = end_y_x1 + point_size;
  float end_y_y2 = end_y_y1 + point_size;
  if (end_y_x1 <= event.x && event.x <= end_y_x2 && end_y_y1 <= event.y && event.y <= end_y_y2)
  {
    _hovered_point = -1;
    _hovered_slope = -1;
    _hovered_end_y = true;
    _hovered_start_y = false;
    setMouseCursor(MouseCursor::DraggingHandCursor);
  }

  if (_hovered_point != prev_hovered_point || _hovered_slope != prev_hovered_slope || 
    _hovered_start_y != prev_hovered_start_y || _hovered_end_y != prev_hovered_end_y)
  {
    repaint();
  }
}

void
mseg_editor::paint(Graphics& g)
{
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

  calc_sorted_points();

  // bg
  g.setColour(_lnf->colors().mseg_background);
  g.fillRect(getLocalBounds());

  // filler
  g.setColour(_lnf->colors().mseg_area);
  g.fillRect(x, y + h, w, padding);

  // grid
  g.setColour(_lnf->colors().mseg_grid);
  g.drawRect(getLocalBounds(), 1.0f);

  // start to point 0
  g.setColour(_lnf->colors().mseg_line);
  make_slope_path(x, y, w, h, { 0.0f, start_y }, _sorted_points[0], 0, false, sloped_path);
  g.strokePath(sloped_path, PathStrokeType(line_thickness));
  g.setColour(_lnf->colors().mseg_area);
  make_slope_path(x, y, w, h, { 0.0f, start_y }, _sorted_points[0], 0, true, sloped_path);
  g.fillPath(sloped_path);

  // point marker
  g.setColour(_hovered_start_y ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size);
  g.setColour(_hovered_start_y ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size, 1);

  // point marker
  g.setColour(_hovered_point == 0 ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x + w * _sorted_points[0].first - point_size / 2, y + h - h * _sorted_points[0].second - point_size / 2, point_size, point_size);
  g.setColour(_hovered_point == 0 ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(x + w * _sorted_points[0].first - point_size / 2, y + h - h * _sorted_points[0].second - point_size / 2, point_size, point_size, 1);

  // slope marker
  slope_marker_x = x + (0.0f + _sorted_points[0].first) * 0.5f * w - point_size / 2;
  slope_marker_y = y + h - h * sloped_y_pos(0.5f, 0, start_y, _sorted_points[0].second) - point_size / 2;
  g.setColour(_hovered_slope == 0 ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);

  // mid sections
  for (int i = 1; i < _sorted_points.size(); i++)
  {
    // point n - 1 to n
    g.setColour(_lnf->colors().mseg_line);
    make_slope_path(x, y, w, h, _sorted_points[i - 1], _sorted_points[i], i, false, sloped_path);
    g.strokePath(sloped_path, PathStrokeType(line_thickness));
    g.setColour(_lnf->colors().mseg_area);
    make_slope_path(x, y, w, h, _sorted_points[i - 1], _sorted_points[i], i, true, sloped_path);
    g.fillPath(sloped_path);

    // point marker
    g.setColour(_hovered_point == i ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
    g.fillEllipse(x + w * _sorted_points[i].first - point_size / 2, y + h - h * _sorted_points[i].second - point_size / 2, point_size, point_size);
    g.setColour(_hovered_point == i ? _lnf->colors().mseg_line : _lnf->colors().mseg_point.withAlpha(0.5f));
    g.drawEllipse(x + w * _sorted_points[i].first - point_size / 2, y + h - h * _sorted_points[i].second - point_size / 2, point_size, point_size, 1);

    // slope marker
    slope_marker_x = x + (_sorted_points[i - 1].first + _sorted_points[i].first) * 0.5f * w - point_size / 2;
    slope_marker_y = y + h - h * sloped_y_pos(0.5f, i, _sorted_points[i - 1].second, _sorted_points[i].second) - point_size / 2;
    g.setColour(_hovered_slope == i ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
    g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);
  }

  // last to end point
  g.setColour(_lnf->colors().mseg_line);
  make_slope_path(x, y, w, h, _sorted_points[_sorted_points.size() - 1], { 1.0f, end_y }, _sorted_points.size(), false, sloped_path);
  g.strokePath(sloped_path, PathStrokeType(line_thickness));
  g.setColour(_lnf->colors().mseg_area);
  make_slope_path(x, y, w, h, _sorted_points[_sorted_points.size() - 1], { 1.0f, end_y }, _sorted_points.size(), true, sloped_path);
  g.fillPath(sloped_path);

  // point marker
  g.setColour(_hovered_end_y ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size);
  g.setColour(_hovered_end_y ? _lnf->colors().mseg_line: _lnf->colors().mseg_point);
  g.drawEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size, 1);

  // slope marker
  slope_marker_x = x + (_sorted_points[_sorted_points.size() - 1].first + 1.0f) * 0.5f * w - point_size / 2;
  slope_marker_y = y + h - h * sloped_y_pos(0.5f, _sorted_points.size(), _sorted_points[_sorted_points.size() - 1].second, end_y) - point_size / 2;
  g.setColour(_hovered_slope == _sorted_points.size() ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);
}

}