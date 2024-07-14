#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/gui/components.hpp>

#include <memory>

using namespace juce;

namespace plugin_base {

void 
add_and_make_visible(
  juce::Component& parent, juce::Component& child)
{
  // This binds it's own visibility.
  if(dynamic_cast<binding_component*>(&child))
    parent.addChildComponent(child);
  else
    parent.addAndMakeVisible(child);
}

juce::Colour
color_to_grayscale(juce::Colour const& c)
{
  float gray = 0.299 * c.getRed() + 0.587 * c.getGreen() + 0.114 * c.getBlue();
  auto rgb = (juce::uint8)(std::clamp((int)(gray), 0, 255));
  return Colour(rgb, rgb, rgb, c.getAlpha());
}

juce::ScaledImage
make_drag_source_image(Font const& font, std::string const& text, Colour text_color)
{
  float margin = 4.0f;
  float text_h = font.getHeight();
  float text_w = font.getStringWidthFloat(String(text)) + 4;

  Image image(Image::PixelFormat::ARGB, text_w + margin, text_h + margin, true);
  Graphics g(image);
  g.setColour(Colours::white);
  g.fillRoundedRectangle(0, 0, text_w + margin - 1, text_h + margin, 2);
  g.setColour(text_color);
  g.drawRoundedRectangle(0, 0, text_w + margin - 1, text_h + margin, 2, 1);
  g.drawText(text, margin / 2, margin / 2, text_w, text_h, Justification::centredBottom, false);
  return ScaledImage(image);
}

}