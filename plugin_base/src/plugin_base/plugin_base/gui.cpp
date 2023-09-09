#include <plugin_base/gui.hpp>
#include <vector>

using namespace juce;

namespace plugin_base {

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
    {
      auto slider = new Slider();
      slider->setSliderStyle(Slider::LinearBarVertical);
      addAndMakeVisible(slider);
    }
  }
  resized();
}

void
plugin_gui::paint(Graphics& g)
{
  g.fillAll(Colours::green); // todo black
  for(int i = 0; i < getChildren().size(); i++)
    getChildComponent(i)->paint(g);
}

void 
plugin_gui::resized()
{
  Grid grid;
  int c = 1;
  grid.templateRows.add(Grid::TrackInfo(Grid::Fr(1)));
  for (int m = 0; m < _desc.modules.size(); m++)
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      grid.templateColumns.add(Grid::TrackInfo(Grid::Fr(1)));
      grid.items.add(GridItem(getChildComponent(c)).withArea(1, c++));
    }
  } 
  grid.performLayout(getLocalBounds());
}

}