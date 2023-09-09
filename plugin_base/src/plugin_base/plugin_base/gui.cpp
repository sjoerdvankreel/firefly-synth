#include <plugin_base/gui.hpp>
#include <plugin_base/topo.hpp>
#include <vector>

using namespace juce;

namespace plugin_base {

class param_slider:
public Slider
{
  param_topo const* const _topo;
public: 
  param_slider(param_topo const* topo);
};

param_slider::
param_slider(param_topo const* topo): _topo(topo)
{
  setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
  if(!_topo->is_real()) setRange(_topo->min, _topo->max, 1);
  else setMinAndMaxValues(_topo->min, _topo->max, dontSendNotification);
  switch (topo->display)
  {
  case param_display::vslider:
    setSliderStyle(Slider::LinearVertical);
    break;
  case param_display::hslider:
    setSliderStyle(Slider::LinearHorizontal);
    break;
  case param_display::knob:
    setSliderStyle(Slider::RotaryVerticalDrag);
    break;
  default:
    // TODO 
    break;
  }
}

plugin_gui::
plugin_gui(plugin_topo_factory factory) :
_desc(factory)
{
  setOpaque(true);
  setSize(_desc.topo.gui_default_width, _desc.topo.gui_default_width / _desc.topo.gui_aspect_ratio);
  for (int m = 0; m < _desc.modules.size(); m++) // todo dont leak
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
      addAndMakeVisible(new param_slider(_desc.modules[m].params[p].topo));
  }
  resized();
}

void 
plugin_gui::resized()
{
  Grid grid;
  int c = 0;
  grid.templateRows.add(Grid::TrackInfo(Grid::Fr(1)));
  for (int m = 0; m < _desc.modules.size(); m++)
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      GridItem item(getChildComponent(c));
      item.row.end = 2;
      item.row.start = 1;
      item.column.start = c + 1;
      item.column.end = c + 2;
      c++;
      grid.items.add(item);
      grid.templateColumns.add(Grid::TrackInfo(Grid::Fr(1)));
    }
  } 
  grid.performLayout(getLocalBounds());
}

}