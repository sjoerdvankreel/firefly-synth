#include <plugin_base/gui/lnf.hpp>

using namespace juce;

namespace plugin_base {

lnf::
lnf(plugin_topo_gui const* gui_topo) :
_gui_topo(gui_topo)
{
  setColour(lnf::tab_bar_background, Colour(0xFF222222));
  setColour(lnf::tab_button_background, Colour(0xFF333333));
  setColour(TabbedButtonBar::ColourIds::tabTextColourId, Colour(0xFFFF8844));
  setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::tabOutlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::frontOutlineColourId, Colours::transparentBlack);
}

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
lnf::drawButtonText(Graphics& g, TextButton& button, bool, bool)
{
  Font font(getTextButtonFont(button, button.getHeight()));
  g.setFont(font);
  int id = button.getToggleState() ? TextButton::textColourOnId: TextButton::textColourOffId;
  g.setColour(button.findColour(id).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
  const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
  const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;
  const int fontHeight = roundToInt(font.getHeight() * 0.6f);
  const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
  const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
  const int textWidth = button.getWidth() - leftIndent - rightIndent;
  if (textWidth > 0)
    g.drawText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, false);
}

void 
lnf::drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
  Path path;
  int const fixedHeight = _gui_topo->font_height + 6;
  int const comboTop = height < fixedHeight ? 0: (height - fixedHeight) / 2;
  auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
  Rectangle<int> boxBounds(0, comboTop, width, fixedHeight);
  g.setColour(box.findColour(ComboBox::backgroundColourId));
  g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
  g.setColour(box.findColour(ComboBox::outlineColourId));
  g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);
  Rectangle<int> arrowZone(width - 30, 0, 20, height);
  path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
  path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
  path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);
  g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
  g.strokePath(path, PathStrokeType(2.0f));
}

int	
lnf::getTabButtonBestWidth(TabBarButton& b, int)
{
  int result = _gui_topo->module_tab_width;
  if(b.getIndex() == 0) result += _gui_topo->module_header_width;
  return result;
}

void 
lnf::drawTabbedButtonBarBackground(TabbedButtonBar& bar, juce::Graphics& g)
{
  g.setColour(findColour(tab_bar_background));
  g.fillRoundedRectangle(bar.getLocalBounds().toFloat(), _gui_topo->module_corner_radius);
}

void
lnf::getIdealPopupMenuItemSize(String const& text, bool separator, int standardHeight, int& w, int& h)
{
  LookAndFeel_V4::getIdealPopupMenuItemSize(text, separator, standardHeight, w, h);
  h = _gui_topo->font_height + 8;
}

void 
lnf::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
  float lighten = button.getToggleState() || isMouseOver? _gui_topo->lighten: 0;
  if (button.getIndex() > 0)
  {
    g.setColour(findColour(tab_button_background).brighter(lighten));
    g.fillRect(button.getActiveArea());
    g.setFont(_gui_topo->font());
    g.setColour(button.findColour(TabbedButtonBar::tabTextColourId).brighter(lighten));
    g.drawText(button.getButtonText(), button.getTextArea(), Justification::centred, false);
    return;
  }

  auto const& header = button.getTabbedButtonBar().getTitle();
  auto headerArea = button.getActiveArea();
  auto buttonArea = headerArea.removeFromRight(_gui_topo->module_tab_width);
  int radius = _gui_topo->module_corner_radius;
  g.setColour(findColour(tab_button_background));
  g.fillRoundedRectangle(headerArea.toFloat(), radius);
  headerArea.removeFromLeft(radius);
  g.fillRect(headerArea.toFloat());
  auto textArea = button.getTextArea();
  textArea.removeFromLeft(radius + 2);
  g.setFont(_gui_topo->font());
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId));
  g.drawText(header, textArea, Justification::left, false);

  if(button.getTabbedButtonBar().getNumTabs() == 1) return;
  buttonArea.removeFromLeft(1);
  g.setColour(findColour(tab_button_background).brighter(lighten));
  g.fillRect(buttonArea);
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId).brighter(lighten));
  g.drawText(button.getButtonText(), buttonArea, Justification::centred, false);
}

}