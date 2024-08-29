#include <plugin_base/shared/tuning.hpp>

namespace plugin_base {

// must match engine_tuning_mode
std::vector<list_item>
engine_tuning_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{CB268630-186C-46E0-9AAC-FC17923A0005}", "No Tuning", "Microtuning disabled");
  result.emplace_back("{0A4A5F76-33C4-417F-9282-4B3F54B940E7}", "Note Before", "Tuning on-note, before modulation");
  result.emplace_back("{7D47FA4A-7109-4C8F-ABDC-66826D8ED637}", "Note After", "Tuning on-note, after modulation");
  result.emplace_back("{2FD02D1C-54F1-4588-A3A5-6C3E9BD8321F}", "Cont. Before", "Tuning continuous, before modulation");
  result.emplace_back("{E799343B-EBF5-41DF-B14F-7AE0C6B0F83D}", "Cont. After", "Tuning continuous, after modulation");
  return result;
}

}