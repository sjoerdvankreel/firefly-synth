#include <plugin_base/shared/tuning.hpp>
#include <plugin_base/shared/io_user.hpp>
#include <plugin_base/shared/utility.hpp>

#include <mutex>
#include <thread>
#include <cassert>

namespace plugin_base {

static std::mutex _mutex = {};
static engine_tuning_mode _global_tuning_mode = (engine_tuning_mode)-1;
static std::vector<global_tuning_mode_changed_handler*> _global_tuning_mode_changed_handlers = {};

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

static engine_tuning_mode
get_global_tuning_mode_from_user_settings(std::string const& vendor, std::string const& full_name)
{
  std::vector<std::string> all_ids;
  std::vector<list_item> all_items = engine_tuning_mode_items();
  for (int i = 0; i < all_items.size(); i++)
    all_ids.push_back(all_items[i].id);
  std::string result_id = user_io_load_list(vendor, full_name, user_io::base, user_state_tuning_key, engine_tuning_mode_items()[1].id, all_ids);
  for (int i = 0; i < all_items.size(); i++)
    if (all_items[i].id == result_id)
      return (engine_tuning_mode)i;
  return engine_tuning_mode_on_note_before_mod;
}

void
add_global_tuning_mode_changed_handler(global_tuning_mode_changed_handler* handler)
{
  std::lock_guard<std::mutex> guard(_mutex);
  _global_tuning_mode_changed_handlers.push_back(handler);
}

void
remove_global_tuning_mode_changed_handler(global_tuning_mode_changed_handler* handler)
{
  std::lock_guard<std::mutex> guard(_mutex);
  auto iter = std::find(_global_tuning_mode_changed_handlers.begin(), _global_tuning_mode_changed_handlers.end(), handler);
  if(iter != _global_tuning_mode_changed_handlers.end())
    _global_tuning_mode_changed_handlers.erase(iter);
}

engine_tuning_mode
get_global_tuning_mode(std::string const& vendor, std::string const& full_name)
{
  engine_tuning_mode result;
  std::lock_guard<std::mutex> guard(_mutex);
  result = _global_tuning_mode;
  if (result == -1)
    result = get_global_tuning_mode_from_user_settings(vendor, full_name);
  return result;
}

void
set_global_tuning_mode(std::string const& vendor, std::string const& full_name, int param_index, plain_value mode)
{
  auto items = engine_tuning_mode_items();
  std::lock_guard<std::mutex> guard(_mutex);
  if (_global_tuning_mode != mode.step())
  {
    _global_tuning_mode = (engine_tuning_mode)mode.step();
    user_io_save_list(vendor, full_name, user_io::base, user_state_tuning_key, items[mode.step()].id);
    for (int i = 0; i < _global_tuning_mode_changed_handlers.size(); i++)
      (*_global_tuning_mode_changed_handlers[i])(param_index, mode);
  }
}

}