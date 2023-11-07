#pragma once

#include <plugin_base/desc/plugin.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

class lnf:
public juce::LookAndFeel_V4 {
  
  int const _module = -1;
  int const _section = -1;
  plugin_desc const* const _desc;
  juce::Typeface::Ptr _typeface = {};

  int tab_width() const;
  gui_colors const& colors() const 
  { return _module == -1? _desc->plugin->gui.colors: _desc->plugin->modules[_module].gui.colors; }

public:
  juce::Font font() const;
  int scrollbar_size() const { return 15; }
  int combo_height() const { return _desc->plugin->gui.font_height + 6; }
  lnf(plugin_desc const* desc, int section, int module);

  juce::Font getPopupMenuFont() override { return font(); }
  juce::Font getLabelFont(juce::Label&) override { return font(); }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return font(); }
  juce::Font getSliderPopupFont(juce::Slider&) override { return font(); }
  juce::Font getTextButtonFont(juce::TextButton&, int) override { return font(); }
  juce::Font getTabButtonFont(juce::TabBarButton& b, float) override { return font(); }
  
  juce::Path getTickShape(float) override;
  int	getTabButtonBestWidth(juce::TabBarButton&, int) override;
  void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
  void getIdealPopupMenuItemSize(juce::String const&, bool, int, int& , int&) override;

  void drawLabel(juce::Graphics&, juce::Label& label) override;
  void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
  void drawTabButton(juce::TabBarButton&, juce::Graphics&, bool, bool) override;
  void drawTabbedButtonBarBackground(juce::TabbedButtonBar&, juce::Graphics&) override;
  void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
  void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
  void drawTickBox(juce::Graphics&, juce::Component&, float, float, float, float, bool, bool, bool, bool) override;
  void drawBubble(juce::Graphics&, juce::BubbleComponent&, juce::Point<float> const&, juce::Rectangle<float> const& body) override;
  void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
};

}
