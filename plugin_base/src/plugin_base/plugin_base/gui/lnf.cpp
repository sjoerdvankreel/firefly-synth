#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/gui/controls.hpp>

#include <juce_gui_basics/detail/juce_LookAndFeelHelpers.h>
#include <cassert>

using namespace juce;

namespace plugin_base {

static int const knob_thumb_width = 6;
static int const knob_thumb_height = 7;
static int const slider_thumb_width = 9;
static int const slider_thumb_height = 6;

static void
draw_tabular_cell_bg(Graphics& g, Component* c, float alpha)
{
  g.setColour(Colours::white.withAlpha(alpha));
  g.fillRoundedRectangle(c->getLocalBounds().reduced(1).toFloat(), 3);
}

static void 
draw_conic_arc(
  Graphics&g, float left, float top, float size, float start_angle, 
  float end_angle, Colour color1, Colour color2, int segment_count, 
  float min, float max, float stroke)
{
  float overlap = 0.01;
  float angle_range = end_angle - start_angle;
  for (int i = (int)(min * segment_count); i < segment_count; i++)
  {
    Path path;
    if ((float)i / segment_count >= max) break;
    g.setColour(color1.interpolatedWith(color2, (float)i / (segment_count - 1)));
    float this_start_angle = start_angle + (float)i / segment_count * angle_range;
    float this_end_angle = start_angle + (float)(i + 1) / segment_count * angle_range;
    if (i < segment_count - 1) this_end_angle += overlap;
    path.addArc(left, top, size, size, this_start_angle, this_end_angle, true);
    g.strokePath(path, PathStrokeType(stroke));
  }
}

lnf::
lnf(plugin_desc const* desc, int custom_section, int module_section, int module) :
_desc(desc), _custom_section(custom_section), _module_section(module_section), _module(module)
{
  assert(module_section == -1 || module >= 0);
  assert(custom_section == -1 || module == -1);

  auto font_path = get_resource_location(desc->config) / resource_folder_ui / desc->plugin->gui.typeface_file_name;
  std::vector<char> typeface = file_load(font_path);
  assert(typeface.size());
  _typeface = Typeface::createSystemTypefaceFor(typeface.data(), typeface.size());
  assert(-1 <= module && module < (int)_desc->plugin->modules.size());

  auto control_text_high = colors().control_text.brighter(_desc->plugin->gui.lighten);
  auto control_bg_high = colors().control_background.brighter(_desc->plugin->gui.lighten);

  setColour(Label::ColourIds::textColourId, colors().label_text);

  setColour(ToggleButton::ColourIds::textColourId, colors().control_text);
  setColour(ToggleButton::ColourIds::tickColourId, colors().control_tick);

  setColour(TextButton::ColourIds::textColourOnId, control_text_high);
  setColour(TextButton::ColourIds::buttonOnColourId, control_bg_high);
  setColour(TextButton::ColourIds::textColourOffId, colors().control_text);
  setColour(TextButton::ColourIds::buttonColourId, colors().control_background);

  setColour(TextEditor::ColourIds::textColourId, colors().edit_text);
  setColour(TextEditor::ColourIds::backgroundColourId, colors().control_background);
  setColour(CaretComponent::ColourIds::caretColourId, colors().edit_text);

  setColour(TabbedButtonBar::ColourIds::tabTextColourId, colors().tab_text);
  setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::tabOutlineColourId, Colours::transparentBlack);
  setColour(TabbedButtonBar::ColourIds::frontOutlineColourId, Colours::transparentBlack);

  setColour(ComboBox::ColourIds::textColourId, colors().control_text);
  setColour(ComboBox::ColourIds::arrowColourId, colors().control_tick);
  setColour(ComboBox::ColourIds::outlineColourId, colors().control_outline);
  setColour(ComboBox::ColourIds::backgroundColourId, colors().control_background);
  setColour(ComboBox::ColourIds::focusedOutlineColourId, colors().control_outline);

  setColour(ScrollBar::ColourIds::thumbColourId, colors().scrollbar_thumb);
  setColour(ScrollBar::ColourIds::backgroundColourId, colors().scrollbar_background);

  setColour(PopupMenu::ColourIds::textColourId, colors().control_text);
  setColour(PopupMenu::ColourIds::backgroundColourId, colors().control_background);
  setColour(PopupMenu::ColourIds::highlightedTextColourId, colors().control_text.brighter(_desc->plugin->gui.lighten));
  setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, colors().control_background.brighter(_desc->plugin->gui.lighten));
}

gui_colors const& 
lnf::colors() const
{
  if(_custom_section != -1)
    return _desc->plugin->gui.custom_sections[_custom_section].colors;
  if(_module != -1)
    return  _desc->plugin->modules[_module].gui.colors;
  return _desc->plugin->gui.colors;
}

Font 
lnf::font() const
{
  Font result(_typeface);
  result.setHeight(_desc->plugin->gui.font_height);
  result.setStyleFlags(_desc->plugin->gui.font_flags);
  return result;
}

Font 
lnf::getLabelFont(Label& label) 
{ 
  Font result = font();
  auto fit = dynamic_cast<autofit_label*>(&label);
  if (fit)
  {
    if (fit->bold()) result = result.boldened();
    if(fit->font_height() != -1) result = result.withHeight(fit->font_height());
  }
  return result;
}

int
lnf::tab_width() const
{
  assert(_module_section != -1);
  auto const& section = _desc->plugin->gui.module_sections[_module_section];
  return section.tabbed ? -1 : _desc->plugin->gui.module_tab_width;
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

int	
lnf::getTabButtonBestWidth(TabBarButton& b, int)
{ 
  int result = tab_width();
  if(result == -1) return b.getTabbedButtonBar().getWidth() / b.getTabbedButtonBar().getNumTabs();
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
lnf::drawBubble(Graphics& g, BubbleComponent& c, Point<float> const& pos, Rectangle<float> const& body)
{
  g.setColour(colors().control_background);
  g.fillRoundedRectangle(body, 2);
  g.setColour(colors().bubble_outline);
  g.drawRoundedRectangle(body, 2, 1);
}

void 
lnf::drawTooltip(Graphics& g, String const& text, int w, int h)
{
  Rectangle<int> bounds(w, h);
  g.setColour(colors().control_background);
  g.fillRect(bounds.toFloat());
  g.setColour(colors().bubble_outline);
  g.drawRect(bounds.toFloat().reduced(0.5f, 0.5f), 1.0f);
  auto layout = detail::LookAndFeelHelpers::layoutTooltipText(text, findColour(TooltipWindow::textColourId));
  layout.draw(g, Rectangle<float>(w, h));
}

void 
lnf::drawTextEditorOutline(juce::Graphics& g, int w, int h, TextEditor& te)
{
  auto cornerSize = 6.0f;
  if (!te.isEnabled()) return;
  if (dynamic_cast<AlertWindow*> (te.getParentComponent()) != nullptr) return;
  if (te.hasKeyboardFocus(true) && !te.isReadOnly())
    g.setColour(te.findColour(TextEditor::focusedOutlineColourId));
  else
    g.setColour(te.findColour(TextEditor::outlineColourId));
  g.drawRoundedRectangle(0, 0, w, h, cornerSize, 2);
}

void
lnf::drawTickBox(
  Graphics& g, Component& c, float x, float y, float w, float h,
  bool ticked, bool isEnabled, bool highlighted, bool down)
{
  Rectangle<float> tickBounds(x, y, w, h);
  g.setColour(colors().control_outline);
  g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);
  if (!ticked) return;
  auto tick = getTickShape(0.67f);
  if(c.isEnabled()) g.setColour(c.findColour(ToggleButton::tickColourId));
  g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(4, 5).toFloat(), true));
}

void 
lnf::drawPopupMenuItemWithOptions(
  Graphics& g, Rectangle<int> const& area, bool highlighted, 
  PopupMenu::Item const& item, PopupMenu::Options const& options)
{
  PopupMenu::Item new_item = item;
  if(item.itemID == -1)
    new_item.isEnabled = true; // just for painting submenu headers, not actually enabled
  LookAndFeel_V4::drawPopupMenuItemWithOptions(g, area, highlighted, new_item, options);
}

void
lnf::drawScrollbar(Graphics& g, ScrollBar& bar, int x, int y, int w, int h,
  bool vertical, int pos, int size, bool over, bool down)
{
  g.setColour(findColour(ScrollBar::ColourIds::backgroundColourId));
  g.fillRect(bar.getLocalBounds());

  Rectangle<int> thumbBounds;
  if (vertical) thumbBounds = { x, pos, w, size };
  else thumbBounds = { pos, y, size, h };
  auto c = bar.findColour(ScrollBar::ColourIds::thumbColourId);
  g.setColour(over ? c.brighter(0.25f) : c);
  g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 2.0f);
}

void
lnf::drawLabel(Graphics& g, Label& label)
{
  g.fillAll(label.findColour(Label::backgroundColourId));

  if (auto afl = dynamic_cast<autofit_label*>(&label))
    if (afl->tabular())
      draw_tabular_cell_bg(g, &label, 0.075f);

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

void
lnf::drawButtonText(Graphics& g, TextButton& button, bool, bool)
{
  Font font(getTextButtonFont(button, button.getHeight()));
  g.setFont(font);
  int id = button.getToggleState() ? TextButton::textColourOnId : TextButton::textColourOffId;
  g.setColour(button.findColour(id).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
  const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
  const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;
  const int fontHeight = roundToInt(font.getHeight() * 0.6f);
  const int leftIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
  const int rightIndent = jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
  const int textWidth = button.getWidth() - leftIndent - rightIndent;
  if (textWidth > 0)
    g.drawText(button.getButtonText(), leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2, Justification::centred, false);
  
  if(!dynamic_cast<menu_button*>(&button)) return;
  
  Path arrow;
  float w = 6;
  float h = 4;
  float x = button.getWidth() - 5 - w;
  float y = button.getHeight() / 2 - h / 2 + 1;
  arrow.startNewSubPath(x, y);
  arrow.lineTo(x + w, y);
  arrow.lineTo(x + w / 2, y + h);
  arrow.closeSubPath();
  g.setColour(colors().edit_text);
  g.fillPath(arrow);
}

void
lnf::drawComboBox(Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
  bool tabular = false;
  if (auto ps = dynamic_cast<param_combobox*>(&box))
    if (ps->param()->param->gui.tabular)
    {
      tabular = true;
      draw_tabular_cell_bg(g, &box, 0.05f);
    }

  Path path;
  int arrowPad = 4;
  int arrowWidth = 6;
  int arrowHeight = 4;
  int const fixedHeight = combo_height(tabular) - (tabular? 4: 0);
  int const comboTop = height < fixedHeight ? 0 : (height - fixedHeight) / 2;
  auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
  Rectangle<int> boxBounds(tabular? 2: 0, comboTop, width - (tabular? 4: 0), fixedHeight);
  g.setColour(Colours::white.withAlpha(0.125f));
  g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);
  path.startNewSubPath(width - arrowWidth - arrowPad, height / 2 - arrowHeight / 2 + 1);
  path.lineTo(width - arrowWidth / 2 - arrowPad, height / 2 + arrowHeight / 2 + 1);
  path.lineTo(width - arrowPad, height / 2 - arrowHeight / 2 + 1);
  path.closeSubPath();
  g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
  g.fillPath(path);
}

void 
lnf::drawToggleButton(Graphics& g, ToggleButton& tb, bool highlighted, bool down)
{
  bool tabular = false;
  if (auto ps = dynamic_cast<param_toggle_button*>(&tb))
    if (ps->param()->param->gui.tabular)
      tabular = true;
  if(tabular)
    draw_tabular_cell_bg(g, &tb, 0.05f);

  int left = 0;
  if(tabular) 
    left = tb.getWidth() / 2 - toggle_height(tabular) / 2;

  int pad = tabular? 3: 1;
  int height = tb.getHeight();
  int const fixedHeight = toggle_height(tabular);
  int const toggleTop = height < fixedHeight ? 0 : (height - fixedHeight) / 2;
  Rectangle<int> boxBounds(left + pad, toggleTop + pad, fixedHeight - pad * 2, fixedHeight - pad * 2);
  g.setColour(Colours::white.withAlpha(0.125f));
  g.fillEllipse(boxBounds.toFloat());
  g.setColour(findColour(ComboBox::outlineColourId).darker());
  g.drawEllipse(boxBounds.toFloat(), 1);
  if (!tb.getToggleState()) return;
  if (tb.isEnabled()) g.setColour(tb.findColour(ToggleButton::tickColourId));
  else g.setColour(tb.findColour(ToggleButton::tickDisabledColourId));
  g.fillEllipse(boxBounds.toFloat().reduced(5.0f, 5.0f));
}

void 
lnf::drawTabButton(TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
  int radius = _desc->plugin->gui.module_corner_radius;
  int strip_left = radius + 2;
  bool is_section = _module_section != -1 && _desc->plugin->gui.module_sections[_module_section].tabbed;
  auto justify = is_section ? Justification::left : Justification::centred;
  
  float button_lighten = button.getToggleState() || isMouseOver ? _desc->plugin->gui.lighten : 0;
  auto text_color = (is_section || button.getToggleState()) ? colors().tab_text : colors().tab_text_inactive;
  g.setColour(colors().tab_button.brighter(button_lighten));

  // no header, evenly distributed, left tab has rounded corners
  // right tab always has rounded corners
  bool left_no_header = button.getIndex() == 0 && tab_width() == -1;
  bool right_most = button.getIndex() > 0 && button.getIndex() == button.getTabbedButtonBar().getNumTabs() - 1;
  if (left_no_header || right_most)
  {
    Path path;
    auto rect = button.getActiveArea().toFloat();
    path.addRoundedRectangle(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), 
      radius, radius, left_no_header, right_most, left_no_header, right_most);
    g.fillPath(path);
  }

  // any tab not left or rightmost
  if (!left_no_header && !right_most && button.getIndex() > 0)
    g.fillRect(button.getActiveArea());

  // fill text for all tab types excluding left with header
  if(left_no_header || button.getIndex() > 0)
  {
    g.setFont(font());
    auto text_area = button.getTextArea();
    if (is_section) text_area.removeFromLeft(strip_left);
    g.setColour(text_color);
    g.drawText(button.getButtonText(), text_area, justify, false);
    return;
  }

  // leftmost tab with header
  auto const& header = button.getTabbedButtonBar().getTitle();
  auto headerArea = button.getActiveArea().toFloat();
  auto buttonArea = headerArea.removeFromRight(tab_width());
  g.setColour(colors().tab_button);
  if(button.getTabbedButtonBar().getNumTabs() == 1)
    g.fillRoundedRectangle(headerArea, radius);
  else
  {
    Path path;
    path.addRoundedRectangle(
      headerArea.getX(), headerArea.getY(), headerArea.getWidth(), 
      headerArea.getHeight(), radius, radius, true, false, true, false);
    g.fillPath(path);
  }

  auto textArea = button.getTextArea();
  textArea.removeFromLeft(radius + 2);
  g.setFont(font());
  g.setColour(button.findColour(TabbedButtonBar::tabTextColourId));
  g.drawText(header, textArea, Justification::left, false);

  // case header only
  if(button.getTabbedButtonBar().getNumTabs() == 1) return;

  // case tab header and first button
  buttonArea.removeFromLeft(1);
  g.setColour(colors().tab_button.brighter(button_lighten));
  g.fillRect(buttonArea);
  if (is_section) buttonArea.removeFromLeft(strip_left);
  g.setColour(text_color);
  g.drawText(button.getButtonText(), buttonArea, justify, false);
}

void 
lnf::drawRotarySlider(Graphics& g, int, int, int, int, float pos, float, float, Slider& s)
{
  float stroke = 5;
  int conic_count = 256;

  bool tabular = false;
  if (auto ps = dynamic_cast<param_slider*>(&s))
    if (ps->param()->param->gui.tabular)
      tabular = true;
  if(tabular)
    draw_tabular_cell_bg(g, &s, 0.05f);

  float scale_factor = 1;
  float size_base = s.getHeight();
  if(tabular) 
  {
    size_base = 0.9 * std::min(s.getHeight(), s.getWidth());
    scale_factor = size_base / s.getHeight();
  }

  float padding = tabular? 3: 5;
  float size = size_base - padding - stroke / 2;
  float left = s.getWidth() - size - padding;
  if(tabular) left = (s.getWidth() - size) / 2;
  float top = (s.getHeight() - size) / 2;

  bool bipolar = s.getMinimum() < 0;
  float end_angle = (180 + 340) * pi32 / 180;
  float start_angle = (180 + 20) * pi32 / 180;
  float angle_range = end_angle - start_angle;
  auto track1 = colors().knob_track1;
  auto track2 = colors().knob_track2;
  auto background1 = colors().knob_background1;
  auto background2 = colors().knob_background2;

  if (!s.isEnabled())
  {
    track1 = color_to_grayscale(track1);
    track2 = color_to_grayscale(track2);
    background1 = color_to_grayscale(background1);
    background2 = color_to_grayscale(background2);
  }

  if(!bipolar)
  {
    draw_conic_arc(g, left, top, size, start_angle, end_angle, background1, background2, conic_count, 0, 1.0f, stroke);
    draw_conic_arc(g, left, top, size, start_angle, end_angle, track1, track2, conic_count, 0, pos, stroke);
  }
  else
  {
    draw_conic_arc(g, left, top, size, start_angle, start_angle + angle_range / 2, background2, background1, conic_count / 2, 0, 1.0f, stroke);
    draw_conic_arc(g, left, top, size, start_angle + angle_range / 2, end_angle, background1, background2, conic_count / 2, 0, 1.0f, stroke);
    if (pos >= 0.5f) draw_conic_arc(g, left, top, size, start_angle + angle_range / 2, end_angle, track1, track2, conic_count / 2, 0, (pos - 0.5f) * 2, stroke);
    else draw_conic_arc(g, left, top, size, start_angle, start_angle + angle_range / 2, track2, track1, conic_count / 2, pos * 2, 1, stroke);
  }

  // modulatable indicator
  if(auto ps = dynamic_cast<param_slider*>(&s))
  {
    if(ps->param()->param->dsp.can_modulate(ps->param()->info.slot))
    {
      auto indicator = background1.interpolatedWith(background2, 0.33);
      if(!s.isEnabled()) indicator = indicator.darker();
      g.setColour(indicator);
      g.fillEllipse(left + size / 4, top + size / 4, size / 2, size / 2);
    }
  }

  Path thumb;
  float thumb_end_angle = 340 * pi32 / 180;
  float thumb_start_angle = 20 * pi32 / 180;
  float thum_angle_range = thumb_end_angle - thumb_start_angle;
  auto thumb_color = colors().slider_thumb;
  if (!s.isEnabled()) thumb_color = color_to_grayscale(thumb_color);
  g.setColour(thumb_color);

  float thumb_top = top + size - knob_thumb_height * scale_factor * 1.25;
  float thumb_left = left + size / 2 - knob_thumb_width * scale_factor / 2;
  thumb.startNewSubPath(thumb_left, thumb_top);
  thumb.lineTo(thumb_left + knob_thumb_width * scale_factor / 2, thumb_top + knob_thumb_height * scale_factor);
  thumb.lineTo(thumb_left + knob_thumb_width * scale_factor, thumb_top);
  thumb.closeSubPath();
  auto transform = AffineTransform::rotation(thumb_start_angle + pos * thum_angle_range, left + size / 2, top + size / 2);
  thumb.applyTransform(transform);
  g.fillPath(thumb);
}

void 	
lnf::drawLinearSlider(Graphics& g, int x, int y, int w, int h, float p, float, float, Slider::SliderStyle style, Slider& s)
{
  float pos = (p - x) / w;
  int const fixedHeight = 5;
  assert(style == Slider::SliderStyle::LinearHorizontal);

  // in table mode dont align right against the next one
  // normally thats not a point because theres labels in between
  int padh = 0;
  if(auto ps = dynamic_cast<param_slider*>(&s))
  {
    if(ps->param()->param->gui.tabular)
    {
      padh = 2;
      draw_tabular_cell_bg(g, &s, 0.05f);
    }
  }

  float left = slider_thumb_width / 2 + padh / 2;
  float top = (s.getHeight() - fixedHeight) / 2;
  float width = s.getWidth() - slider_thumb_width - padh;
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

  auto thumb_color = colors().slider_thumb;
  if (!s.isEnabled()) thumb_color = color_to_grayscale(thumb_color);

  // modulatable indicator
  if (auto ps = dynamic_cast<param_slider*>(&s))
  {
    if (ps->param()->param->dsp.can_modulate(ps->param()->info.slot))
    {
      auto indicator = outline1.interpolatedWith(outline2, 0.33);
      if (!s.isEnabled()) indicator = indicator.darker();
      g.setColour(thumb_color);
      g.fillEllipse(left + width / 2 - height / 2 + 1, top + 1, height - 2, height - 2);
    }
  }

  Path thumb;
  float thumb_left = width * pos;
  float thumb_top = s.getHeight() / 2;
  g.setColour(thumb_color);
  thumb.startNewSubPath(thumb_left, thumb_top + slider_thumb_height);
  thumb.lineTo(thumb_left + slider_thumb_width / 2, thumb_top);
  thumb.lineTo(thumb_left + slider_thumb_width, thumb_top + slider_thumb_height);
  thumb.closeSubPath();
  g.fillPath(thumb);
}

}