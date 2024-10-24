#include <plugin_base/gui/mseg_editor.hpp>

using namespace juce;

namespace plugin_base {

mseg_editor::
mseg_editor(
  plugin_gui* gui, int module_index, int module_slot, 
  int start_y_param, int end_y_param, int on_param, int x_param, int y_param):
_gui(gui), 
_module_index(module_index), _module_slot(module_slot), 
_start_y_param(start_y_param), _end_y_param(end_y_param),
_on_param(on_param), _x_param(x_param), _y_param(y_param)
{
  // todo loads of validation
  // need at least 1 point !? maybe ==> yes
}

void
mseg_editor::paint(Graphics& g)
{
  float w = getLocalBounds().getWidth();
  float h = getLocalBounds().getHeight();
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

  g.setColour(Colours::red);
  g.fillRect(getLocalBounds());
  g.setColour(Colours::blue);

  // start to point 0
  g.drawLine(0, h - start_y * h, w * points[0].first, h - h * points[0].second);

  // mid sections
  for (int i = 1; i < points.size(); i++)
    g.drawLine(w * points[i - 1].first, h - h * points[i - 1].second, w * points[i].first, h - h * points[i].second);

  // last to end point
  g.drawLine(w * points[points.size() - 1].first, h - h * points[points.size() - 1].second, w - 1, h - end_y * h);
}

}