#include <plugin_base/dsp/block/plugin.hpp>

namespace plugin_base {

// must match engine_tuning_mode
std::vector<list_item>
engine_tuning_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{CB268630-186C-46E0-9AAC-FC17923A0005}", "No Tuning");
  result.emplace_back("{0A4A5F76-33C4-417F-9282-4B3F54B940E7}", "On Note Before Mod");
  result.emplace_back("{7D47FA4A-7109-4C8F-ABDC-66826D8ED637}", "On Note After Mod Linear");
  result.emplace_back("{4D574539-A6E4-446E-AD41-AC5A2E0B5769}", "On Note After Mod Log");
  result.emplace_back("{2FD02D1C-54F1-4588-A3A5-6C3E9BD8321F}", "Continuous Before Mod");
  result.emplace_back("{E799343B-EBF5-41DF-B14F-7AE0C6B0F83D}", "Continuous After Mod Linear");
  result.emplace_back("{FF5FAD5D-2AE1-46BC-92EC-7DBE38D052A7}", "Continuous After Mod Log");
  return result;
}

  
}