#include <plugin_base/gui/listeners.hpp>

namespace plugin_base {

void
gui_listener::gui_changed(int index, plain_value plain)
{
  gui_begin_changes(index);
  gui_changing(index, plain);
  gui_end_changes(index);
}

}