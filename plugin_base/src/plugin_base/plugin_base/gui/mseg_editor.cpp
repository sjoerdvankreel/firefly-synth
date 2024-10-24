#include <plugin_base/gui/mseg_editor.hpp>

using namespace juce;

namespace plugin_base {

void
mseg_editor::paint(Graphics& g)
{
  g.setColour(Colours::red);
  g.fillRect(getLocalBounds());
}

}