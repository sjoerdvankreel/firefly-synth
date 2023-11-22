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

std::filesystem::path
get_resource_location(format_config const* config, 
  std::string const& folder, std::string const& file_name)
{
  File file(File::getSpecialLocation(File::currentExecutableFile));
  return config->resources_folder(file.getFullPathName().toStdString()) / folder / file_name;
}

}