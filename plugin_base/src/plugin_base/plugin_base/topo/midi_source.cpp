#include <plugin_base/topo/midi_source.hpp>

namespace plugin_base {

void
midi_source::validate() const
{
  tag.validate();
  if(id.message == midi_msg_cc)
    assert(0 <= id.cc_number && id.cc_number < 128);
  else 
    assert(id.cc_number == 0);
  assert(id.message == midi_msg_cc || id.message == midi_msg_pb || id.message == midi_msg_cp);
}

}