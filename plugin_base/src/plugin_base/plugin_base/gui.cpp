#include <plugin_base/gui.hpp>

using namespace juce;

namespace plugin_base {

plugin_gui::
plugin_gui(plugin_topo_factory factory) :
_desc(factory)
{
  setOpaque(true);
  setSize(_desc.topo.gui_default_width, _desc.topo.gui_default_width / _desc.topo.gui_aspect_ratio);
}

void 
plugin_gui::paint(Graphics& g)
{
  g.fillAll(juce::Colours::aliceblue);
}

}