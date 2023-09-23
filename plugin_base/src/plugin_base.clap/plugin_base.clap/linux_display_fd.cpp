#if (defined __linux__) || (defined  __FreeBSD__)
#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_gui_basics/juce_gui_basics.h>

namespace plugin_base::clap {

int 
current_display_fd()
{
  auto system = juce::XWindowSystem::getInstance();
  return ConnectionNumber(system->getDisplay());
}

}
#endif