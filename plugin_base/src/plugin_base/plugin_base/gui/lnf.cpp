#include <plugin_base/gui/lnf.hpp>

#include <cassert>
#include <fstream>

using namespace juce;

namespace plugin_base {

lnf::
lnf(plugin_desc const* desc, int module) :
_desc(desc), _module(module)
{
  File file(File::getSpecialLocation(File::currentExecutableFile));
  std::filesystem::path path = desc->config->resources_folder(file.getFullPathName().toStdString());
  std::vector<char> typeface = file_load(path / desc->plugin->gui.typeface_file_name);
  _typeface = Typeface::createSystemTypefaceFor(typeface.data(), typeface.size());
  assert(module < (int)_desc->plugin->modules.size());
  if(module < 0) return;

  setColour(TabbedButtonBar::ColourIds::tabTextColourId, module_gui().colors.tab_text);
  setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::tabOutlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::frontOutlineColourId, Colours::transparentBlack);

  setColour(ComboBox::ColourIds::textColourId, module_gui().colors.dropdown_text);
  setColour(ComboBox::ColourIds::arrowColourId, module_gui().colors.dropdown_tick);
  setColour(ComboBox::ColourIds::outlineColourId, module_gui().colors.dropdown_outline);
  setColour(ComboBox::ColourIds::backgroundColourId, module_gui().colors.dropdown_background);
  setColour(ComboBox::ColourIds::focusedOutlineColourId, module_gui().colors.dropdown_outline);

  setColour(PopupMenu::ColourIds::textColourId, module_gui().colors.dropdown_text);
  setColour(PopupMenu::ColourIds::backgroundColourId, module_gui().colors.dropdown_background);
  setColour(PopupMenu::ColourIds::highlightedTextColourId, module_gui().colors.dropdown_text.brighter(_desc->plugin->gui.lighten));
  setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, module_gui().colors.dropdown_background.brighter(_desc->plugin->gui.lighten));
}

Font 
lnf::font() const
{
  Font result(_typeface);
  result.setHeight(_desc->plugin->gui.font_height);
  result.setStyleFlags(_desc->plugin->gui.font_flags);
  return result;
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

Path 
lnf::getTickShape(float h)
{
  Path result;
  result.addArc(0, 0, h, h, 0, 2 * pi32 + 1);
  return result;
}

void 
lnf::positionComboBoxText(ComboBox& box, Label& label)
{
  label.setBounds(1, 1, box.getWidth() - 10, box.getHeight() - 2);
  label.setFont(getComboBoxFont(box));
}

void 
lnf::drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
  Path path;
  int arrowPad = 4;
  int arrowWidth = 6;
  int arrowHeight = 4;
  int const fixedHeight = _desc->plugin->gui.font_height + 6;
  int const comboTop = height < fixedHeight ? 0: (height - fixedHeight) / 2;
  auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
  Rectangle<int> boxBounds(0, comboTop, width, fixedHeight);
  g.setColour(box.findColour(ComboBox::backgroundColourId));
  g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
  g.setColour(box.findColour(ComboBox::outlineColourId));
  g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);
  path.startNewSubPath(width - arrowWidth - arrowPad, height / 2 - arrowHeight / 2 + 1);
  path.lineTo(width - arrowWidth / 2 - arrowPad, height / 2 + arrowHeight / 2 + 1);
  path.lineTo(width - arrowPad, height / 2 - arrowHeight / 2 + 1);
  g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
  g.fillPath(path);
}

int	
lnf::getTabButtonBestWidth(TabBarButton& b, int)
{
  int result = _desc->plugin->gui.module_tab_width;
  if(b.getIndex() == 0) result += _desc->plugin->gui.module_header_width;
  return result;
}

void 
lnf::drawTabbedButtonBarBackground(TabbedButtonBar& bar, juce::Graphics& g)
{
  g.setColour(module_gui().colors.tab_header);
  g.fillRoundedRectangle(bar.getLocalBounds().toFloat(), _desc->plugin->gui.module_corner_radius);
}

void
lnf::getIdealPopupMenuItemSize(String const& text, bool separator, int standardHeight, int& w, int& h)
{
  LookAndFeel_V4::getIdealPopupMenuItemSize(text, separator, standardHeight, w, h);
  h = _desc->plugin->gui.font_height + 8;
}

void 
lnf::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
  float lighten = button.getToggleState() || isMouseOver? _desc->plugin->gui.lighten: 0;
  if (button.getIndex() > 0)
  {
    g.setColour(module_gui().colors.tab_button.brighter(lighten));
    g.fillRect(button.getActiveArea());
    g.setFont(font());
    g.setColour(button.findColour(TabbedButtonBar::tabTextColourId).brighter(lighten));
    g.drawText(button.getButtonText(), button.getTextArea(), Justification::centred, false);
    return;
  }

  auto const& header = button.getTabbedButtonBar().getTitle();
  auto headerArea = button.getActiveArea();
  auto buttonArea = headerArea.removeFromRight(_desc->plugin->gui.module_tab_width);
  int radius = _desc->plugin->gui.module_corner_radius;
  g.setColour(module_gui().colors.tab_button);
  g.fillRoundedRectangle(headerArea.toFloat(), radius);
  headerArea.removeFromLeft(radius);
  g.fillRect(headerArea.toFloat());
  auto textArea = button.getTextArea();
  textArea.removeFromLeft(radius + 2);
  g.setFont(font());
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId));
  g.drawText(header, textArea, Justification::left, false);

  if(button.getTabbedButtonBar().getNumTabs() == 1) return;
  buttonArea.removeFromLeft(1);
  g.setColour(module_gui().colors.tab_button.brighter(lighten));
  g.fillRect(buttonArea);
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId).brighter(lighten));
  g.drawText(button.getButtonText(), buttonArea, Justification::centred, false);
}

}