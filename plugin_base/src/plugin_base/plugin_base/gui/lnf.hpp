#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

inline float const default_font_height = 14.0f;
inline juce::String const default_font_typeface = "Verdana";

class lnf:
public juce::LookAndFeel_V4 {
  
  juce::Font defaultFont(int styleFlags)
  { return { juce::Font(default_font_typeface, default_font_height, juce::Font::FontStyleFlags::plain) }; }

public:
  juce::Font getPopupMenuFont() override { return defaultFont(0); }
  juce::Font getLabelFont(juce::Label&) override { return defaultFont(0); }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return defaultFont(0); }
  juce::Font getTextButtonFont(juce::TextButton&, int) override { return defaultFont(0); }
  juce::Font getTabButtonFont(juce::TabBarButton& b, float) override { return defaultFont(0); }

  void drawLabel(juce::Graphics& g, juce::Label& label) override;
  int	getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override;
  void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;
  void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override;
};

}
