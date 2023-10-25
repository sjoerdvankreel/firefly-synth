#include <plugin_base/gui/lnf.hpp>

using namespace juce;

namespace plugin_base {

void 
lnf::drawLabel(Graphics& g, Label& label)
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
  } else if (label.isEnabled())
    g.setColour(label.findColour(Label::outlineColourId));
  g.drawRect(label.getLocalBounds());
}

void 
lnf::drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
  int const fixedHeight = default_font_height + 6;
  int const comboTop = height < fixedHeight ? 0: (height - fixedHeight) / 2;

  auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
  Rectangle<int> boxBounds(0, comboTop, width, fixedHeight);

  g.setColour(box.findColour(ComboBox::backgroundColourId));
  g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
  g.setColour(box.findColour(ComboBox::outlineColourId));
  g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

  Path path;
  Rectangle<int> arrowZone(width - 30, 0, 20, height);
  path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
  path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
  path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);

  g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
  g.strokePath(path, PathStrokeType(2.0f));
}

void 
lnf::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
  auto justify = button.getIndex() == 0? Justification::right: Justification::centred;
  g.setColour(button.getTabBackgroundColour());
  g.setColour(Colours::red);
  g.fillRect(button.getActiveArea());
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId));
  g.setColour(Colours::blue);
  g.drawText(button.getButtonText(), button.getTextArea(), justify, false);
}

}