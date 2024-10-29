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
  int start_y_param, int count_param, int x_param, int y_param, int slope_param):
_gui(gui), _lnf(lnf),
_module_index(module_index), _module_slot(module_slot),
_start_y_param(start_y_param), _count_param(count_param),
_x_param(x_param), _y_param(y_param), _slope_param(slope_param)
{
  assert(gui != nullptr);
  assert(lnf != nullptr);
  auto const& topo = *gui->automation_state()->desc().plugin;
  auto const& param_list = topo.modules[module_index].params;
  (void)param_list;

  auto is_linear_unit = [](param_topo const& pt) { 
    return pt.domain.type == domain_type::identity && pt.domain.min == 0 && pt.domain.max == 1; };
  assert(is_linear_unit(param_list[start_y_param]));
  assert(is_linear_unit(param_list[x_param]));
  assert(is_linear_unit(param_list[y_param]));
  assert(is_linear_unit(param_list[slope_param]));
  assert(param_list[start_y_param].info.slot_count == 1);
  assert(param_list[x_param].info.slot_count >= 1);
  assert(param_list[y_param].info.slot_count == param_list[x_param].info.slot_count);
  assert(param_list[slope_param].info.slot_count == param_list[x_param].info.slot_count); 

  _max_seg_count = param_list[x_param].info.slot_count;
  _current_seg_count = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _count_param, 0).step();
  _gui->automation_state()->add_listener(_module_index, _module_slot, _count_param, 0, this);
  _gui->automation_state()->add_listener(_module_index, _module_slot, _start_y_param, 0, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_x_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _x_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_y_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _y_param, i, this);
  for (int i = 0; i < _gui->automation_state()->desc().plugin->modules[_module_index].params[_slope_param].info.slot_count; i++)
    _gui->automation_state()->add_listener(_module_index, _module_slot, _slope_param, i, this);

  _gui_start_y = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _start_y_param, 0).real();
  for (int i = 0; i < _current_seg_count; i++)
  {
    mseg_seg seg = {};
    seg.x = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _x_param, i).real();
    seg.y = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _y_param, i).real();
    seg.slope = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _slope_param, i).real();
    _gui_segs.push_back(seg);
  }
}

mseg_editor::
~mseg_editor()
{
  _gui->automation_state()->remove_listener(_module_index, _module_slot, _count_param, 0, this);
  _gui->automation_state()->remove_listener(_module_index, _module_slot, _start_y_param, 0, this);
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
  //repaint(); TODO
}

float 
mseg_editor::sloped_y_pos(float pos, int seg) const
{
  float slope = _gui_segs[seg].slope;
  float y1 = seg == 0 ? _gui_start_y : _gui_segs[seg - 1].y;
  float y2 = _gui_segs[seg].y;
  double const slope_min = exp_slope_min;
  double const slope_max = (1.0 - exp_slope_min);
  double const slope_range = slope_max - slope_min;
  double const slope_bounded = exp_slope_min + slope_range * slope;
  double const exp = std::log(slope_bounded) / std::log(0.5);
  return y1 + std::pow(pos, exp) * (y2 - y1); 

  // TODO DSP NOT needs to sort!
}

void 
mseg_editor::make_slope_path(
  float x, float y, float w, float h,
  int seg, bool closed, juce::Path& path) const
{
  path = {};
  float x1_norm = seg == 0? 0.0f: _gui_segs[seg - 1].x;
  float x2_norm = _gui_segs[seg].x;
  float y1_norm = seg == 0? _gui_start_y: _gui_segs[seg - 1].y;

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
    float y_this_pos_norm = sloped_y_pos(pos, seg);
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
mseg_editor::mouseDoubleClick(MouseEvent const& event)
{
  int hit_seg;
  bool hit_start_y;
  bool hit_seg_slope;
  bool hit = hit_test(event, hit_start_y, hit_seg, hit_seg_slope);
  
  if (hit && hit_start_y) return;
  if (hit && hit_seg == _gui_segs.size() - 1) return;

  // case join  
  if(hit && !hit_seg_slope)
  {
    if (_gui_segs.size() > 1)
    {
      _gui_segs.erase(_gui_segs.begin() + hit_seg);
      repaint();
    }
    return;
  }

  // case splice
  float const x = padding;
  float const w = getLocalBounds().getWidth() - padding * 2.0f;

  if (event.x <= x) return;
  if (event.x >= x + w) return;

  float new_norm_x = (event.x - x) / w;
  for(int i = 0; i < _gui_segs.size(); i++)
    if (new_norm_x < _gui_segs[i].x)
    {
      mseg_seg new_seg;
      new_seg.x = new_norm_x;
      new_seg.y = 0.5f;
      new_seg.slope = 0.5f;
      _gui_segs.insert(_gui_segs.begin() + i, new_seg);
      repaint();
      break;
    }
}

void
mseg_editor::mouseDrag(MouseEvent const& event)
{
  if (isDragAndDropActive()) return;
  if (!hit_test(event, _drag_start_y, _drag_seg, _drag_seg_slope)) return;
  
  Image image(Image::PixelFormat::ARGB, point_size, point_size, true);
  Graphics g(image);
  if (_drag_seg_slope)
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

  // TODO startchanges et all
  Point<int> offset(image.getWidth() / 2 + point_size, image.getHeight() / 2 + point_size);
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
  /* TODO
  if (_dragging_start_y) _gui->param_end_changes(_module_index, _module_slot, _start_y_param, 0);
  else if (_dragging_end_y) _gui->param_end_changes(_module_index, _module_slot, _end_y_param, 0);
  else if (_dragging_point != -1) _gui->param_end_changes(_module_index, _module_slot, _y_param, _sorted_points[_dragging_point].param_index);
  else if (_dragging_slope != -1) _gui->param_end_changes(_module_index, _module_slot, _slope_param, 
    _dragging_slope == _sorted_points.size() ? _sorted_points[_dragging_slope - 1].param_index + 1 : _sorted_points[_dragging_slope].param_index);
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
  */
}

void 
mseg_editor::itemDragMove(juce::DragAndDropTarget::SourceDetails const& details)
{
  //float const x = padding;
  float const y = padding;
  //float const w = getLocalBounds().getWidth() - padding * 2.0f;
  float const h = getLocalBounds().getHeight() - padding * 2.0f;
  float drag_y_amt = 1.0f - std::clamp((details.localPosition.y - y) / h, 0.0f, 1.0f);

  if (_drag_start_y)
  {
    _gui_start_y = drag_y_amt;
    repaint();
    return;
  }

  // TODO everything
}

bool
mseg_editor::hit_test(
  MouseEvent const& e, bool& hit_start_y,
  int& hit_seg, bool& hit_seg_slope) const
{
  float const x = padding;
  float const y = padding;
  float const start_y = _gui_start_y;
  float const w = getLocalBounds().getWidth() - padding * 2.0f;
  float const h = getLocalBounds().getHeight() - padding * 2.0f;

  hit_seg = -1;
  hit_start_y = false;
  hit_seg_slope = false;

  float start_y_x1 = x - point_size / 2;
  float start_y_y1 = y + h - start_y * h - point_size / 2;
  float start_y_x2 = start_y_x1 + point_size;
  float start_y_y2 = start_y_y1 + point_size;
  if (start_y_x1 <= e.x && e.x <= start_y_x2 && start_y_y1 <= e.y && e.y <= start_y_y2)
  {
    hit_start_y = true;
    return true;
  }

  for (int i = 0; i < _gui_segs.size(); i++)
  {
    float prev_x = i == 0 ? 0.0f : _gui_segs[i - 1].x;
    float this_slope_x1 = x + (prev_x + _gui_segs[i].x) * 0.5f * w - point_size / 2;
    float this_slope_y1 = y + h - h * sloped_y_pos(0.5f, i) - point_size / 2;
    float this_slope_x2 = this_slope_x1 + point_size;
    float this_slope_y2 = this_slope_y1 + point_size;
    if (this_slope_x1 <= e.x && e.x <= this_slope_x2 && this_slope_y1 <= e.y && e.y <= this_slope_y2)
    {
      hit_seg = i;
      hit_seg_slope = true;
      return true;
    }

    float this_point_x1 = x + w * _gui_segs[i].x - point_size / 2;
    float this_point_y1 = y + h - h * _gui_segs[i].y - point_size / 2;
    float this_point_x2 = this_point_x1 + point_size;
    float this_point_y2 = this_point_y1 + point_size;
    if (this_point_x1 <= e.x && e.x <= this_point_x2 && this_point_y1 <= e.y && e.y <= this_point_y2)
    {
      hit_seg = i;
      hit_seg_slope = false;
      return true;
    }
  }

  return false;
}

void
mseg_editor::mouseMove(MouseEvent const& event)
{
  int hit_seg;
  bool hit_start_y;
  bool hit_seg_slope;

  setTooltip("");
  setMouseCursor(MouseCursor::ParentCursor);
  if (!hit_test(event, hit_start_y, hit_seg, hit_seg_slope))
    return;

  setMouseCursor(MouseCursor::DraggingHandCursor);
  auto const& topo = *_gui->automation_state()->desc().plugin;
  if (hit_start_y) setTooltip(topo.modules[_module_index].params[_start_y_param].info.tag.display_name);
  else if (hit_seg != -1)
  {
    if(hit_seg_slope) setTooltip(topo.modules[_module_index].params[_slope_param].info.tag.display_name + " " + std::to_string(hit_seg + 1));
    else setTooltip(topo.modules[_module_index].params[_y_param].info.tag.display_name + " " + std::to_string(hit_seg + 1));
  }

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
  float start_y = _gui_start_y;
  float end_y = _gui_segs[_gui_segs.size() - 1].y;

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

  // start y point marker
  //g.setColour(_hit_test_start_y ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
  g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
  g.fillEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size);
  //g.setColour(_hit_test_start_y ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
  g.setColour(_lnf->colors().mseg_point);
  g.drawEllipse(x - point_size / 2, y + h - start_y * h - point_size / 2, point_size, point_size, 1);

  // actual segments
  for (int i = 0; i < _gui_segs.size(); i++)
  {
    g.setColour(_lnf->colors().mseg_line);
    make_slope_path(x, y, w, h, i, false, sloped_path);
    g.strokePath(sloped_path, PathStrokeType(line_thickness));
    g.setColour(_lnf->colors().mseg_area);
    make_slope_path(x, y, w, h, i, true, sloped_path);
    g.fillPath(sloped_path);

    // point marker
    //g.setColour(_hit_test_point == i ? _lnf->colors().mseg_line.withAlpha(0.5f) : _lnf->colors().mseg_point.withAlpha(0.5f));
    g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
    g.fillEllipse(x + w * _gui_segs[i].x - point_size / 2, y + h - h * _gui_segs[i].y - point_size / 2, point_size, point_size);
    //g.setColour(_hit_test_point == i ? _lnf->colors().mseg_line : _lnf->colors().mseg_point.withAlpha(0.5f));
    g.setColour(_lnf->colors().mseg_point.withAlpha(0.5f));
    g.drawEllipse(x + w * _gui_segs[i].x - point_size / 2, y + h - h * _gui_segs[i].y - point_size / 2, point_size, point_size, 1);

    // slope marker
    float prev_x = i == 0 ? 0.0f : _gui_segs[i - 1].x;
    slope_marker_x = x + (prev_x + _gui_segs[i].x) * 0.5f * w - point_size / 2;
    slope_marker_y = y + h - h * sloped_y_pos(0.5f, i) - point_size / 2;
    //g.setColour(_hit_test_slope == i ? _lnf->colors().mseg_line : _lnf->colors().mseg_point);
    g.setColour(_lnf->colors().mseg_point);
    g.drawEllipse(slope_marker_x, slope_marker_y, point_size, point_size, 1.0f);
  }
}

}