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
make_drag_source_image(Font const& font, std::string const& text, Colour border_color)
{
  float margin_w = 8.0f;
  float margin_h = 4.0f;
  float text_h = font.getHeight();
  float text_w = font.getStringWidthFloat(String(text));

  Image image(Image::PixelFormat::ARGB, text_w + margin_w, text_h + margin_h, true);
  Graphics g(image);
  g.setFont(font);
  g.setColour(Colours::black);
  g.fillRect(0.0f, 0.0f, text_w + margin_w - 1, text_h + margin_h);
  g.setColour(border_color);
  g.drawRect(Rectangle<float>(0.0f, 0.0f, text_w + margin_w - 1, text_h + margin_h).reduced(0.5f), 1.0f);
  g.setColour(Colours::white);
  g.drawText(text, margin_w / 2, margin_h / 2, text_w, text_h, Justification::centredBottom, false);
  return ScaledImage(image);
}

}