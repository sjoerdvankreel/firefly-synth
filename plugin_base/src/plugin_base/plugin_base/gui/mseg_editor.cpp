#include <plugin_base/gui/mseg_editor.hpp>

using namespace juce;

namespace plugin_base {

mseg_editor::
mseg_editor(plugin_gui* gui, int module_index, int module_slot, int start_y_param, int end_y_param):
_gui(gui), 
_module_index(module_index), _module_slot(module_slot), 
_start_y_param(start_y_param), _end_y_param(end_y_param)
{
  // todo loads of validation
}

void
mseg_editor::paint(Graphics& g)
{
  float w = getLocalBounds().getWidth();
  float h = getLocalBounds().getHeight();
  float end_y = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _start_y_param, 0).real();
  float start_y = _gui->automation_state()->get_plain_at(_module_index, _module_slot, _start_y_param, 0).real();

  g.setColour(Colours::red);
  g.fillRect(getLocalBounds());
  g.setColour(Colours::blue);
  g.drawLine(0, start_y, w, end_y * h);
}

}