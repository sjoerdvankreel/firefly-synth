#pragma once

#include <plugin_base/topo/domain.hpp>
#include <string>
#include <vector>

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

std::vector<list_item>
engine_tuning_mode_items();

// ok so this is new -- need to load per-user settings into the topo on startup
std::string
get_per_user_engine_tuning_mode_name(std::string const& vendor, std::string const& full_name);

}