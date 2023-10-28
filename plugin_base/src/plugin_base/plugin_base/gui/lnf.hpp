#pragma once

#include <plugin_base/topo/plugin.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base {

class lnf:
public juce::LookAndFeel_V4 {
  
  int const _module = -1;
  plugin_topo const* const _topo;
  juce::Typeface::Ptr _typeface = {};
  module_topo_gui const& module_gui() const { return _topo->modules[_module].gui; }

public:
  juce::Font font() const;
  lnf(plugin_topo const* topo, int module);

  juce::Font getPopupMenuFont() override { return font(); }
  juce::Font getLabelFont(juce::Label&) override { return font(); }
  juce::Font getComboBoxFont(juce::ComboBox&) override { return font(); }
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
};

}
