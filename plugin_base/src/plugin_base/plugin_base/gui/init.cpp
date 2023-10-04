#include <plugin_base/gui/lnf.hpp>
#include <plugin_base/gui/init.hpp>

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
  initialiseJuce_GUI(); 
  _lnf = std::make_unique<lnf>();
  LookAndFeel::setDefaultLookAndFeel(_lnf.get());
}

}