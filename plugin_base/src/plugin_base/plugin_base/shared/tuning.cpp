#include <plugin_base/shared/tuning.hpp>
#include <plugin_base/shared/io_user.hpp>
#include <plugin_base/shared/utility.hpp>

#include <mutex>
#include <thread>
#include <cassert>

namespace plugin_base {

static std::mutex _mutex = {};

// must match engine_tuning_mode
std::vector<list_item>
engine_tuning_mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{CB268630-186C-46E0-9AAC-FC17923A0005}", "No Tuning");
  result.emplace_back("{0A4A5F76-33C4-417F-9282-4B3F54B940E7}", "On Note Before Mod");
  result.emplace_back("{7D47FA4A-7109-4C8F-ABDC-66826D8ED637}", "On Note After Mod");
  result.emplace_back("{2FD02D1C-54F1-4588-A3A5-6C3E9BD8321F}", "Continuous Before Mod");
  result.emplace_back("{E799343B-EBF5-41DF-B14F-7AE0C6B0F83D}", "Continuous After Mod");
  return result;
}

}