#include <plugin_base/topo/support.hpp>
#include <plugin_base/topo/midi_source.hpp>

namespace plugin_base {

void
midi_source::validate() const
{
  tag.validate();
  assert(0 <= id && id < midi_source_count);
}

std::vector<midi_source> 
midi_source::all_sources()
{
  std::vector<midi_source> result;
  std::string pb_id = "{21798073-DBBC-4B24-B8E0-B6C8A23F1AB1}";
  std::string cp_id = "{2AE18C2E-94B6-4C85-B743-1473F233FFE2}";
  std::string cc_id = "{967C457B-6952-4DE7-8520-45FDDF4AC611}";
  for(int i = 0; i < 128; i++)
    result.push_back(make_midi_source(
      make_topo_tag(cc_id + "-" + std::to_string(i), 
      "MIDI CC " + std::to_string(i)), midi_source_cc + i));
  result.push_back(make_midi_source(make_topo_tag(cp_id, "MIDI CP"), midi_source_cp));
  result.push_back(make_midi_source(make_topo_tag(pb_id, "MIDI PB"), midi_source_pb));
  return result;
}

}