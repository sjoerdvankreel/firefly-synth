#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

inline float const default_font_height = 14.0f;
inline juce::String const default_font_typeface = "Cousine";

class lnf:
public juce::LookAndFeel_V4 {
  
  juce::Font defaultFont(int styleFlags)
  { return { juce::Font(default_font_typeface, default_font_height, juce::Font::bold) }; }

public:
  juce::Font getPopupMenuFont() override { return defaultFont(0); }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return defaultFont(0); }
  juce::Font getTabButtonFont(juce::TabBarButton& b, float height) override { return defaultFont(0); }

  void drawLabel(juce::Graphics& g, juce::Label& label) override;
  int	getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override;
  void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool isMouseOver, bool isMouseDown) override;
  void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override;
};

}
