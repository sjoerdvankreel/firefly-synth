#include <plugin_base/gui_lnf.hpp>

using namespace juce;

namespace plugin_base {

void 
gui_lnf::drawLabel(Graphics& g, Label& label)
{
  g.fillAll(label.findColour(Label::backgroundColourId));
  if (!label.isBeingEdited())
  {
    auto alpha = label.isEnabled() ? 1.0f : 0.5f;
    auto area = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());
    g.setFont(getLabelFont(label));
    g.setColour(label.findColour(Label::textColourId).withMultipliedAlpha(alpha));
    g.drawText(label.getText(), area, label.getJustificationType(), false);
    g.setColour(label.findColour(Label::outlineColourId).withMultipliedAlpha(alpha));
  }
  else if (label.isEnabled())
    g.setColour(label.findColour(Label::outlineColourId));
  g.drawRect(label.getLocalBounds());
}

}