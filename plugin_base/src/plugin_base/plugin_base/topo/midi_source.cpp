#include <plugin_base/topo/midi_source.hpp>

namespace plugin_base {

void
midi_source::validate() const
{
  tag.validate();
  if(message == midi_msg_cc) assert(0 <= cc_number && cc_number < 128);
  else assert(cc_number == 0);
  assert(message == midi_msg_cc || message == midi_msg_pb || message == midi_msg_cp);
}

}