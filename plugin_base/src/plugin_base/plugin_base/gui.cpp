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
  layout();
}

void 
plugin_gui::layout()
{
  Grid grid;
  grid.templateRows.add(Grid::TrackInfo());
  for (int m = 0; m < _desc.modules.size(); m++)
  {
    auto const& module = _desc.modules[m];
    for (int p = 0; p < module.params.size(); p++)
    {
      grid.templateColumns.add(Grid::TrackInfo());
      grid.items.add(GridItem(new Slider())); // todo dont leak
    }
  }
  grid.performLayout(getLocalBounds());
}

}