#pragma once

#include <plugin_base/topo/domain.hpp>

#include <string>
#include <vector>
#include <functional>

namespace plugin_base {

inline std::string const user_state_tuning_key = "tuning";

// needs cooperation from the plug
enum engine_tuning_mode {
  engine_tuning_mode_no_tuning, // no microtuning
  engine_tuning_mode_on_note_before_mod, // query at voice start, tune before modulation
  engine_tuning_mode_on_note_after_mod_linear, // query at voice start, tune after modulation, lerp freq
  engine_tuning_mode_on_note_after_mod_log, // query at voice start, tune after modulation, lerp log2freq
  engine_tuning_mode_continuous_before_mod, // query each block, tune before modulation
  engine_tuning_mode_continuous_after_mod_linear, // query each block, tune after modulation, lerp freq
  engine_tuning_mode_continuous_after_mod_log, // query each block, tune after modulation, lerp log2freq
  engine_tuning_mode_count
};

typedef std::function<void(int param_index, plain_value value)> global_tuning_mode_changed_handler;

std::vector<list_item>
engine_tuning_mode_items();

// ok so this is new -
// need to load per-user settings into the topo on startup
// and deal with cross-instance stuff (no cross process for now)
// note getters/setters take a lock, dont call on audio thread
void
add_global_tuning_mode_changed_handler(global_tuning_mode_changed_handler* handler);
void
remove_global_tuning_mode_changed_handler(global_tuning_mode_changed_handler* handler);
engine_tuning_mode
get_global_tuning_mode(std::string const& vendor, std::string const& full_name);
void
set_global_tuning_mode(std::string const& vendor, std::string const& full_name, int param_index, plain_value mode);

}