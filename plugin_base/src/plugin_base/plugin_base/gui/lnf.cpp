#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>

#include <cassert>
#include <fstream>

using namespace juce;

namespace plugin_base {

static int const thumb_width = 8;
static int const thumb_height = 6;

static Path 
create_thumb(int left, int top)
{
  Path result;
  result.startNewSubPath(left, top + thumb_height);
  result.lineTo(left + thumb_width / 2, top);
  result.lineTo(left + thumb_width, top + thumb_height);
  result.closeSubPath();
  return result;
}

static void 
draw_conic_arc(
  Graphics&g, float left, float top, float size, float start_angle, 
  float end_angle, Colour color1, Colour color2, int segment_count, float max)
{
  float overlap = 0.01;
  float angle_range = end_angle - start_angle;
  for (int i = 0; i < segment_count; i++)
  {
    Path path;
    if ((float)i / segment_count >= max) break;
    g.setColour(color1.interpolatedWith(color2, (float)i / (segment_count - 1)));
    float this_start_angle = start_angle + (float)i / segment_count * angle_range;
    float this_end_angle = start_angle + (float)(i + 1) / segment_count * angle_range;
    if (i < segment_count - 1) this_end_angle += overlap;
    path.addArc(left, top, size, size, this_start_angle, this_end_angle, true);
    g.strokePath(path, PathStrokeType(4));
  }
}

lnf::
lnf(plugin_desc const* desc, int module) :
_desc(desc), _module(module)
{
  File file(File::getSpecialLocation(File::currentExecutableFile));
  std::filesystem::path resources = desc->config->resources_folder(file.getFullPathName().toStdString());
  std::vector<char> typeface = file_load(resources / desc->plugin->gui.typeface_file_name);
  assert(typeface.size());
  _typeface = Typeface::createSystemTypefaceFor(typeface.data(), typeface.size());
  assert(-1 <= module && module < (int)_desc->plugin->modules.size());

  auto control_text_high = colors().control_text.brighter(_desc->plugin->gui.lighten);
  auto control_bg_high = colors().control_background.brighter(_desc->plugin->gui.lighten);

  setColour(ToggleButton::ColourIds::textColourId, colors().control_text);
  setColour(ToggleButton::ColourIds::tickColourId, colors().control_tick);

  setColour(TextButton::ColourIds::textColourOnId, control_text_high);
  setColour(TextButton::ColourIds::buttonOnColourId, control_bg_high);
  setColour(TextButton::ColourIds::textColourOffId, colors().control_text);
  setColour(TextButton::ColourIds::buttonColourId, colors().control_background);

  setColour(TextEditor::ColourIds::textColourId, colors().control_text);
  setColour(TextEditor::ColourIds::backgroundColourId, colors().control_background);

  setColour(TabbedButtonBar::ColourIds::tabTextColourId, colors().tab_text);
  setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::tabOutlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::frontOutlineColourId, Colours::transparentBlack);

  setColour(ComboBox::ColourIds::textColourId, colors().control_text);
  setColour(ComboBox::ColourIds::arrowColourId, colors().control_tick);
  setColour(ComboBox::ColourIds::outlineColourId, colors().control_outline);
  setColour(ComboBox::ColourIds::backgroundColourId, colors().control_background);
  setColour(ComboBox::ColourIds::focusedOutlineColourId, colors().control_outline);

  setColour(PopupMenu::ColourIds::textColourId, colors().control_text);
  setColour(PopupMenu::ColourIds::backgroundColourId, colors().control_background);
  setColour(PopupMenu::ColourIds::highlightedTextColourId, colors().control_text.brighter(_desc->plugin->gui.lighten));
  setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, colors().control_background.brighter(_desc->plugin->gui.lighten));
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
  path.closeSubPath();
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
  g.setColour(colors().tab_header);
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
    g.setColour(colors().tab_button.brighter(lighten));
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
  g.setColour(colors().tab_button);
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
  g.setColour(colors().tab_button.brighter(lighten));
  g.fillRect(buttonArea);
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId).brighter(lighten));
  g.drawText(button.getButtonText(), buttonArea, Justification::centred, false);
}

void
lnf::drawBubble(Graphics& g, BubbleComponent& c, Point<float> const& pos, Rectangle<float> const& body)
{
  g.setColour(colors().control_background);
  g.fillRoundedRectangle(body, 2);
  g.setColour(colors().bubble_outline);
  g.drawRoundedRectangle(body, 2, 1);
}

void 
lnf::drawRotarySlider(Graphics& g, int, int, int, int, float pos, float, float, Slider& s)
{
  float path_size = 4;
  float top_margin = 1;
  int conic_count = 256;
  float size = std::min(s.getWidth(), s.getHeight()) - path_size;
  float left = (s.getWidth() - size) / 2;
  float top = (s.getHeight() - size) / 2 + top_margin;

  float end_angle = (180 + 340) * pi32 / 180;
  float start_angle = (180 + 20) * pi32 / 180;
  draw_conic_arc(g, left, top, size, start_angle, end_angle, colors().knob_background1, colors().knob_background2, conic_count, 1.0f);
  draw_conic_arc(g, left, top, size, start_angle, end_angle, colors().knob_track1, colors().knob_track2, conic_count, pos);

  float half_tw = thumb_width / 2;
  float half_th = thumb_height / 2;
  g.setColour(colors().knob_thumb);
  Path thumb(create_thumb(0, 0));
  auto transform = AffineTransform::rotation(pos * 2 * pi32, half_tw, half_th);
  transform = transform.translated(left + size / 2 - half_tw, top + size / 2 - half_th);
  thumb.applyTransform(transform);
  g.fillPath(thumb);
}

void 	
lnf::drawLinearSlider(Graphics& g, int x, int y, int w, int h, float p, float, float, Slider::SliderStyle style, Slider& s)
{
  float pos = (p - x) / w;
  int const fixedHeight = 4;
  assert(style == Slider::SliderStyle::LinearHorizontal);

  float left = thumb_width / 2;
  float top = (s.getHeight() - fixedHeight) / 2;
  float width = s.getWidth() - thumb_width;
  float centerx = left + width / 2;
  float height = fixedHeight;

  auto track1 = colors().slider_track1;
  auto track2 = colors().slider_track2;
  auto outline1 = colors().slider_outline1;
  auto outline2 = colors().slider_outline2;

  bool bipolar = s.getMinimum() < 0;
  g.setColour(colors().slider_background);
  g.fillRoundedRectangle(left, top, width, height, 2);
  if(!bipolar)
  {
    g.setGradientFill(ColourGradient(track1, left, 0, track2, width, 0, false));
    g.fillRoundedRectangle(left, top, pos * width, height, 2);
    g.setGradientFill(ColourGradient(outline1, left, 0, outline2, width, 0, false));
    g.drawRoundedRectangle(left, top, width, height, 2, 1);
  } else
  {
    if (pos >= 0.5)
    {
      g.setGradientFill(ColourGradient(track1, centerx, 0, track2, width, 0, false));
      g.fillRoundedRectangle(centerx, top, (pos - 0.5f) * 2 * width / 2, height, 2);
    } else
    {
      float trackw = (0.5f - pos) * 2 * width / 2;
      g.setGradientFill(ColourGradient(track2, left, 0, track1, centerx, 0, false));
      g.fillRoundedRectangle(centerx - trackw, top, trackw, height, 2);
    }

    Path pl;
    pl.addRoundedRectangle(left, top, width / 2, height, 2, 2, true, false, true, false);
    g.setGradientFill(ColourGradient(outline2, left, 0, outline1, centerx, 0, false));
    g.strokePath(pl, PathStrokeType(1.0f));
    Path pr;
    pr.addRoundedRectangle(centerx, top, width / 2, height, 2, 2, false, true, false, true);
    g.setGradientFill(ColourGradient(outline1, centerx, 0, outline2, centerx + width / 2, 0, false));
    g.strokePath(pr, PathStrokeType(1.0f));
  }

  float thumb_left = width * pos;
  float thumb_top = s.getHeight() / 2;
  auto thumb_color = colors().slider_thumb;
  if (!s.isEnabled()) thumb_color = color_to_grayscale(thumb_color);
  g.setColour(thumb_color);
  g.fillPath(create_thumb(thumb_left, thumb_top));
}

}