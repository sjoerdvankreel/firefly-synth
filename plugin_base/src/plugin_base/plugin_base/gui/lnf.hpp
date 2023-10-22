#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

inline int const default_font_height = 14;

class lnf:
public juce::LookAndFeel_V4 {
public:
  void drawComboBox(juce::Graphics& g, int width, int height, 
    bool, int, int, int, int, juce::ComboBox& box) override;
  void drawLabel(juce::Graphics& g, juce::Label& label) override;

  juce::Font getPopupMenuFont() override { return { default_font_height }; }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return { default_font_height }; }
};

}
