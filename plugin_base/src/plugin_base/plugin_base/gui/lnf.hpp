#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

inline float const default_font_height = 14.0f;
inline juce::String const default_font_typeface = "Verdana";

class lnf:
public juce::LookAndFeel_V4 {
  
  juce::Font defaultFont(int styleFlags)
  { return { juce::Font(default_font_typeface, default_font_height, juce::Font::plain) }; }

public:
  juce::Font getPopupMenuFont() override { return defaultFont(0); }
  juce::Font getLabelFont(juce::Label&) override { return defaultFont(0); }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return defaultFont(0); }
  juce::Font getTextButtonFont(juce::TextButton&, int) override { return defaultFont(0); }
  juce::Font getTabButtonFont(juce::TabBarButton& b, float) override { return defaultFont(0); }

  int	getTabButtonBestWidth(juce::TabBarButton&, int) override;
  void drawLabel(juce::Graphics&, juce::Label& label) override;
  void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
  void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool, bool) override;
  void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
};

}
