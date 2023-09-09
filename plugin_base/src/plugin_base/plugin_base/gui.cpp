#include <plugin_base/gui.hpp>

namespace plugin_base {

plugin_gui::
plugin_gui(plugin_topo_factory factory) :
_desc(factory)
{
  setOpaque(true);
}

}