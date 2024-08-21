#pragma once

#include <plugin_base/topo/domain.hpp>

#include <string>
#include <vector>
#include <functional>

namespace plugin_base {

inline std::string const extra_state_tuning_mode_key = "tuning_mode";

// needs cooperation from the plug
enum engine_tuning_mode {
  engine_tuning_mode_no_tuning, // no microtuning
  engine_tuning_mode_on_note_before_mod, // query at voice start, tune before modulation
  engine_tuning_mode_on_note_after_mod, // query at voice start, tune after modulation
  engine_tuning_mode_continuous_before_mod, // query each block, tune before modulation
  engine_tuning_mode_continuous_after_mod, // query each block, tune after modulation
  engine_tuning_mode_count
};

std::vector<list_item>
engine_tuning_mode_items();
inline std::set<std::string>
tuning_extra_state_keyset() { return { extra_state_tuning_mode_key }; }

}