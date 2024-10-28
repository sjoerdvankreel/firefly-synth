#include <plugin_base/gui/mseg_editor.hpp>

using namespace juce;

namespace plugin_base {

static char const* mseg_magic = "MSEG_MAGIC";
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

  _gui->automation_state()->add_listener(_module_index, _module_slot, _end_y_param, 0, this);
  _gui->automation_state()->add_listener(_module_index, _module_slot, _start_y_param, 0, this);
  for(int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_on_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _on_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_x_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _x_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_y_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _y_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_slope_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _slope_param, i, this);

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

mseg_editor::
~mseg_editor()
{
  _gui->automation_state()->remove_listener(_module_index, _module_slot, _end_y_param, 0, this);
  _gui->automation_state()->remove_listener(_module_index, _module_slot, _start_y_param, 0, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_on_param].info.slot_count; i++)
    _gui->automation_state()->remove_listener(_module_index, _module_slot, _on_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_x_param].info.slot_count; i++)
    _gui->automation_state()->remove_listener(_module_index, _module_slot, _x_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_y_param].info.slot_count; i++)
    _gui->automation_state()->remove_listener(_module_index, _module_slot, _y_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_slope_param].info.slot_count; i++)
    _gui->automation_state()->remove_listener(_module_index, _module_slot, _slope_param, i, this);
}

void 
mseg_editor::state_changed(int index, plain_value plain)
{
  repaint();
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
      mseg_point point;
      point.param_index = i;
      point.x = state->get_plain_at(_module_index, _module_slot, _x_param, i).real();
      point.y = state->get_plain_at(_module_index, _module_slot, _y_param, i).real();
      _sorted_points.push_back(point);
    }

  // dsp also must sort !
  std::sort(_sorted_points.begin(), _sorted_points.end(), [](auto const& l, auto const& r) { return l.x < r.x; });
}

void 
mseg_editor::make_slope_path(
  float x, float y, float w, float h, 
  mseg_point const& from, 
  mseg_point const& to,
  int slope_index, bool closed, Path& path) const
{
  path = {};
  float x1_norm = from.x;
  float x2_norm = to.x;
  float y1_norm = from.y;
  float y2_norm = to.y;

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
  _dragging_point = _hit_test_point;
  _dragging_slope = _hit_test_slope;
  _dragging_end_y = _hit_test_end_y;
  _dragging_start_y = _hit_test_start_y;
}

void
mseg_editor::mouseDoubleClick(MouseEvent const& event)
{
  if (!hit_test(event)) return;

  if (_hit_test_point >= 0)
  {
    _gui->param_changed(_module_index, _module_slot, _on_param, _sorted_points[_hit_test_point].param_index, 0);
    repaint();
  }
}

void
mseg_editor::mouseDrag(MouseEvent const& event)
{
  if (isDragAndDropActive()) return;
  if (_dragging_slope == -1 && _dragging_point == -1 && !_dragging_start_y && !_dragging_end_y) return;
  
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
  if (_dragging_start_y) _gui->param_begin_changes(_module_index, _module_slot, _start_y_param, 0);
  else if (_dragging_end_y) _gui->param_begin_changes(_module_index, _module_slot, _end_y_param, 0);
  else if (_dragging_point != -1) _gui->param_begin_changes(_module_index, _module_slot, _y_param, _sorted_points[_dragging_point].param_index);
  else if (_dragging_slope != -1) _gui->param_begin_changes(_module_index, _module_slot, _slope_param, _sorted_points[_dragging_slope].param_index);
  else assert(false);
  startDragging(String(mseg_magic), this, ScaledImage(image), false, &offset);
}

bool
mseg_editor::isInterestedInDragSource(DragAndDropTarget::SourceDetails const& details)
{
  return details.description == String(mseg_magic);
}

void
mseg_editor::itemDropped(DragAndDropTarget::SourceDetails const& details)
{
  if (_dragging_start_y) _gui->param_end_changes(_module_index, _module_slot, _start_y_param, 0);
  else if (_dragging_end_y) _gui->param_end_changes(_module_index, _module_slot, _end_y_param, 0);
  else if (_dragging_point != -1) _gui->param_end_changes(_module_index, _module_slot, _y_param, _sorted_points[_dragging_point].param_index);
  else if (_dragging_slope != -1) _gui->param_end_changes(_module_index, _module_slot, _slope_param, _sorted_points[_dragging_slope].param_index);
  else assert(false);

  _hit_test_point = -1;
  _hit_test_slope = -1;
  _hit_test_end_y = false;
  _hit_test_start_y = false;

  _dragging_point = -1;
  _dragging_slope = -1;
  _dragging_end_y = false;
  _dragging_start_y = false;

  setMouseCursor(MouseCursor::ParentCursor);
  repaint();
}

void 
mseg_editor::itemDragMove(juce::DragAndDropTarget::SourceDetails const& details)
{
  //float const x = padding;
  float const y = padding;
  //float const w = getLocalBounds().getWidth() - padding * 2.0f;
  float const h = getLocalBounds().getHeight() - padding * 2.0f;

  float drag_y_amt = 1.0f - std::clamp((details.localPosition.y - y) / h, 0.0f, 1.0f);;

  if (_dragging_start_y)
  {
    // todo start/end
    _gui->param_changing(_module_index, _module_slot, _start_y_param, 0, drag_y_amt);
    repaint();
    return;
  }

  if (_dragging_end_y)
  {
    _gui->param_changing(_module_index, _module_slot, _end_y_param, 0, drag_y_amt);
    repaint();
    return;
  }

  if (_dragging_point != -1)
  {
    _gui->param_changing(_module_index, _module_slot, _y_param, _dragging_point, drag_y_amt);
    repaint();
    return;
  }
}

bool
mseg_editor::hit_test(MouseEvent const& e)
{
  float const x = padding;
  float const y = padding;
  float const w = getLocalBounds().getWidth() - padding * 2.0f;
  float const h = getLocalBounds().getHeight() - padding * 2.0f;

  auto const state = _gui->automation_state();
  float end_y = state->get_plain_at(_module_index, _module_slot, _end_y_param, 0).real();
  float start_y = state->get_plain_at(_module_index, _module_slot, _start_y_param, 0).real();

  _hit_test_point = -1;
  _hit_test_slope = -1;
  _hit_test_end_y = false;
  _hit_test_start_y = false;

  calc_sorted_points();

  // todo this is bound to cause overlapping on near points ...

  float start_y_x1 = x - point_size / 2;
  float start_y_y1 = y + h - start_y * h - point_size / 2;
  float start_y_x2 = start_y_x1 + point_size;
  float start_y_y2 = start_y_y1 + point_size;
  if (start_y_x1 <= e.x && e.x <= start_y_x2 && start_y_y1 <= e.y && e.y <= start_y_y2)
  {
    _hit_test_point = -1;
    _hit_test_slope = -1;
    _hit_test_end_y = false;
    _hit_test_start_y = true;
    return true;
  }

  float start_slope_x1 = x + (0.0f + _sorted_points[0].x) * 0.5f * w - point_size / 2;
  float start_slope_y1 = y + h - h * sloped_y_pos(0.5f, 0, start_y, _sorted_points[0].y) - point_size / 2;
  float start_slope_x2 = start_slope_x1 + point_size;
  float start_slope_y2 = start_slope_y1 + point_size;
  if (start_slope_x1 <= e.x && e.x <= start_slope_x2 && start_slope_y1 <= e.y && e.y <= start_slope_y2)
  {
    _hit_test_point = -1;
    _hit_test_slope = 0;
    _hit_test_end_y = false;
    _hit_test_start_y = false;
    return true;
  }

  for (int i = 0; i < _sorted_points.size(); i++)
  {
    float this_point_x1 = x + w * _sorted_points[i].x - point_size / 2;
    float this_point_y1 = y + h - h * _sorted_points[i].y - point_size / 2;
    float this_point_x2 = this_point_x1 + point_size;
    float this_point_y2 = this_point_y1 + point_size;
    if (this_point_x1 <= e.x && e.x <= this_point_x2 && this_point_y1 <= e.y && e.y <= this_point_y2)
    {
      _hit_test_point = i;
      _hit_test_slope = -1;
      _hit_test_end_y = false;
      _hit_test_start_y = false;
      return true;
    }

    if (i > 0)
    {
      float this_slope_x1 = x + (_sorted_points[i - 1].x + _sorted_points[i].x) * 0.5f * w - point_size / 2;
      float this_slope_y1 = y + h - h * sloped_y_pos(0.5f, i, _sorted_points[i - 1].y, _sorted_points[i].y) - point_size / 2;
      float this_slope_x2 = this_slope_x1 + point_size;
      float this_slope_y2 = this_slope_y1 + point_size;
      if (this_slope_x1 <= e.x && e.x <= this_slope_x2 && this_slope_y1 <= e.y && e.y <= this_slope_y2)
      {
        _hit_test_point = -1;
        _hit_test_slope = i;
        _hit_test_end_y = false;
        _hit_test_start_y = false;
        return true;
      }
    }
  }

  float last_slope_x1 = x + (_sorted_points[_sorted_points.size() - 1].x + 1.0f) * 0.5f * w - point_size / 2;
  float last_slope_y1 = y + h - h * sloped_y_pos(0.5f, _sorted_points.size(), _sorted_points[_sorted_points.size() - 1].y, end_y) - point_size / 2;
  float last_slope_x2 = last_slope_x1 + point_size;
  float last_slope_y2 = last_slope_y1 + point_size;
  if (last_slope_x1 <= e.x && e.x <= last_slope_x2 && last_slope_y1 <= e.y && e.y <= last_slope_y2)
  {
    _hit_test_point = -1;
    _hit_test_slope = _sorted_points.size();
    _hit_test_end_y = false;
    _hit_test_start_y = false;
    return true;
  }

  float end_y_x1 = x + w - 1 - point_size / 2;
  float end_y_y1 = y + h - end_y * h - point_size / 2;
  float end_y_x2 = end_y_x1 + point_size;
  float end_y_y2 = end_y_y1 + point_size;
  if (end_y_x1 <= e.x && e.x <= end_y_x2 && end_y_y1 <= e.y && e.y <= end_y_y2)
  {
    _hit_test_point = -1;
    _hit_test_slope = -1;
    _hit_test_end_y = true;
    _hit_test_start_y = false;
    return true;
  }

  return false;
}

void
mseg_editor::mouseMove(MouseEvent const& event)
{
  int prev_hovered_point = _hit_test_point;
  int prev_hovered_slope = _hit_test_slope;
  bool prev_hovered_end_y = _hit_test_end_y;
  bool prev_hovered_start_y = _hit_test_start_y;

  setMouseCursor(MouseCursor::ParentCursor);
  if(hit_test(event))
    setMouseCursor(MouseCursor::DraggingHandCursor);

  if (_hit_test_point != prev_hovered_point || _hit_test_slope != prev_hovered_slope ||
    _hit_test_start_y != prev_hovered_start_y || _hit_test_end_y != prev_hovered_end_y)
    repaint();
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
  g.fillRect(x + w, y + h - end_y * h, padding, end_y * h + padding);
  g.fillRect(0.0f, y + h - start_y * h, padding, start_y * h + padding);

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
  g.setColour(_hit_test_start_y ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size);
  g.setColour(_hit_test_start_y ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size, 1);

  // point marker
  g.setColour(_hit_test_point == 0 ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x + w * _sorted_points[0].x - point_size / 2, y + h - h * _sorted_points[0].y - point_size / 2, point_size, point_size);
  g.setColour(_hit_test_point == 0 ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(x + w * _sorted_points[0].x - point_size / 2, y + h - h * _sorted_points[0].y - point_size / 2, point_size, point_size, 1);

  // slope marker
  slope_marker_x = x + (0.0f + _sorted_points[0].x) * 0.5f * w - point_size / 2;
  slope_marker_y = y + h - h * sloped_y_pos(0.5f, 0, start_y, _sorted_points[0].y) - point_size / 2;
  g.setColour(_hit_test_slope == 0 ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
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
    g.setColour(_hit_test_point == i ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
    g.fillEllipse(x + w * _sorted_points[i].x - point_size / 2, y + h - h * _sorted_points[i].y - point_size / 2, point_size, point_size);
    g.setColour(_hit_test_point == i ? _lnf->colors().mseg_line : _lnf->colors().mseg_point.withAlpha(0.5f));
    g.drawEllipse(x + w * _sorted_points[i].x - point_size / 2, y + h - h * _sorted_points[i].y - point_size / 2, point_size, point_size, 1);

    // slope marker
    slope_marker_x = x + (_sorted_points[i - 1].x + _sorted_points[i].x) * 0.5f * w - point_size / 2;
    slope_marker_y = y + h - h * sloped_y_pos(0.5f, i, _sorted_points[i - 1].y, _sorted_points[i].y) - point_size / 2;
    g.setColour(_hit_test_slope == i ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
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
  g.setColour(_hit_test_end_y ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size);
  g.setColour(_hit_test_end_y ? _lnf->colors().mseg_line: _lnf->colors().mseg_point);
  g.drawEllipse(x + w - 1 - point_size / 2, y + h - end_y * h - point_size / 2, point_size, point_size, 1);

  // slope marker
  slope_marker_x = x + (_sorted_points[_sorted_points.size() - 1].x + 1.0f) * 0.5f * w - point_size / 2;
  slope_marker_y = y + h - h * sloped_y_pos(0.5f, _sorted_points.size(), _sorted_points[_sorted_points.size() - 1].y, end_y) - point_size / 2;
  g.setColour(_hit_test_slope == _sorted_points.size() ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);
}

}