#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base/gui/components.hpp>

#include <memory>

using namespace juce;

namespace plugin_base {

static std::unique_ptr<lnf> _lnf = {};

void 
gui_terminate()
{ 
  LookAndFeel::setDefaultLookAndFeel(nullptr);
  _lnf.reset();
  shutdownJuce_GUI(); 
}

void 
gui_init()
{ 
  bool found = false;
  auto const& default_typeface = default_font_typeface;
  auto available_typefaces = Font::findAllTypefaceNames();
  for(int i = 0; i < available_typefaces.size(); i++)
    if(available_typefaces[i] == default_typeface) { found = true; break;}
  assert(found || !"Default font not found.");
  initialiseJuce_GUI(); 
  _lnf = std::make_unique<lnf>();
  LookAndFeel::setDefaultLookAndFeel(_lnf.get());
}

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

void
gui_listener::gui_changed(int index, plain_value plain) 
{
  gui_begin_changes(index);
  gui_changing(index, plain);
  gui_end_changes(index);
}

}