#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <plugin_base/shared/value.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <filesystem>

namespace plugin_base {

inline char constexpr resource_folder_ui[] = "ui";
inline char constexpr resource_folder_presets[] = "presets";

void 
add_and_make_visible(
  juce::Component& parent, 
  juce::Component& child);

juce::Colour
color_to_grayscale(juce::Colour const& c);

std::filesystem::path
get_resource_location(format_config const* config, 
  std::string const& folder, std::string const& file_name);

}
